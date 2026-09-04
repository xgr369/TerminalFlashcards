#define _CRT_SECURE_NO_WARNINGS
#include "core.h"
#include "common.h"
#include "list.h"
#include "sqlite3.h"
#include <direct.h>
#include <errno.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <conio.h>

#define MAX_PATH 256
#define BUFFER_LIST_CAPACITY 2
#define APP_ERR_INTERNAL 1
#define APP_ERR_DIR_NOTFOUND 2
static const char BASE_PATH[] = "flashcards";

// Compares if the string is equal to the specified span
static int str_eq_span(const char *str, const char *start, size_t n) {
	const char *p1 = str, *p2 = start;
	while (*p1) {
		if (p1 - str >= n) {
			return 0;
		}
		if (*p1 != *p2) {
			return 0;
		}
		p1++;
		p2++;
	}
	if (p1 - str < n) {
		return 0;
	}
	return 1;
}

static int app_collect_files(const char *dir, char ***files, size_t *count) {
	struct _finddata_t data;
	char pattern[MAX_PATH];
	char path[MAX_PATH];

	strcpy(pattern, dir);
	strcat(pattern, "\\*");
	intptr_t handle = _findfirst(pattern, &data);
	if (handle == -1) {
		return APP_ERR_INTERNAL;
	}
	do {
		if (strcmp(data.name, ".") == 0 || strcmp(data.name, "..") == 0) {
			continue;
		}
		strcpy(path, dir);
		strcat(path, "/");
		strcat(path, data.name);
		if (data.attrib & _A_SUBDIR) {
			if (app_collect_files(path, files, count)) {
				_findclose(handle);
				return APP_ERR_INTERNAL;
			}
		} else {
			char *file = malloc(strlen(path) + 1);
			if (!file) {
				_findclose(handle);
				return APP_ERR_INTERNAL;
			}
			strcpy(file, path);
			char **new_files = realloc(*files, (*count + 1) * sizeof(char *));
			if (!new_files) {
				free(file);
				_findclose(handle);
				return APP_ERR_INTERNAL;
			}
			*files = new_files;
			(*files)[*count] = file;
			(*count)++;
		}
	} while (_findnext(handle, &data) == 0);
	_findclose(handle);
	return 0;
}

static int app_delete_scheduler_entry(sqlite3 *db, const char *path) {
	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(db, "DELETE FROM scheduler WHERE path = ?;", -1, &stmt, NULL) != SQLITE_OK) {
		return APP_ERR_INTERNAL;
	}
	sqlite3_bind_text(stmt, 1, path, -1, SQLITE_TRANSIENT);
	if (sqlite3_step(stmt) != SQLITE_DONE) {
		sqlite3_finalize(stmt);
		return APP_ERR_INTERNAL;
	}
	sqlite3_finalize(stmt);
	return 0;
}

static int app_get_due_files(AppState *s, sqlite3 *db, List *files) {
	int result = 0;

	List unused_files;
	if (list_init(&unused_files, sizeof(char *), BUFFER_LIST_CAPACITY)) {
		return APP_ERR_INTERNAL;
	}

	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(db, "SELECT path FROM scheduler WHERE due_date <= ? AND path LIKE ?;", -1, &stmt, NULL) != SQLITE_OK) {
		result = APP_ERR_INTERNAL;
		goto app_get_due_files_cleanup_unused_files;
	}
	sqlite3_bind_int64(stmt, 1, (sqlite3_int64)time(NULL));
	char prefix[MAX_PATH + 2];
	snprintf(prefix, sizeof(prefix), "%s%%", s->path);
	sqlite3_bind_text(stmt, 2, prefix, -1, SQLITE_TRANSIENT);

	int step_result;
	while ((step_result = sqlite3_step(stmt)) == SQLITE_ROW) {
		const char *path = (const char *)sqlite3_column_text(stmt, 0);
		if (!path) {
			continue;
		}
		char *path_copy = malloc(strlen(path) + 1);

		if (!path_copy) {
			result = APP_ERR_INTERNAL;
			goto app_get_due_files_cleanup_stmt;
		}
		strcpy(path_copy, path);

		if (file_exists(path_copy)) {
			printf("debug--found due file: %s\n", path_copy);
			if (list_push(files, &path_copy)) {
				free(path_copy);
				result = APP_ERR_INTERNAL;
				goto app_get_due_files_cleanup_stmt;
			}
		} else {
			printf("debug--found delete file: %s\n", path_copy);
			if (list_push(&unused_files, &path_copy)) {
				free(path_copy);
				result = APP_ERR_INTERNAL;
				goto app_get_due_files_cleanup_stmt;
			}
		}
		
	}
	if (step_result != SQLITE_DONE) {
		result = APP_ERR_INTERNAL;
		goto app_get_due_files_cleanup_stmt;
	}

	// Delete unused files
	for (size_t i = 0; i < unused_files.length; i++) {
		char *path = *(char **)list_at(&unused_files, i);
		result = app_delete_scheduler_entry(db, path);
		if (result) {
			goto app_get_due_files_cleanup_unused_files;
		}
	}

	// Free statement
app_get_due_files_cleanup_stmt:
	sqlite3_finalize(stmt);

	// Free unused_files
app_get_due_files_cleanup_unused_files:
	for (size_t i = 0; i < unused_files.length; i++) {
		free(*(char **)list_at(&unused_files, i));
	}
	list_free(&unused_files);
	return result;
}

static char *app_parse_path(AppState *s, const char *path) {
	size_t root_len = strlen(s->path);
	size_t path_len = strlen(path);

	char *real_path = malloc(root_len + 1 + path_len + 1);
	if (!real_path) {
		return APP_ERR_INTERNAL;
	}
	char *p = real_path;
	memcpy(p, s->path, root_len);
	p += root_len;
	*p++ = '/';
	const char *start = path;
	while (*start) {
		while (*start == '/' || *start == '\\') {
			start++;
		}
		if (!*start) {
			break;
		}
		const char *end = start;
		while (*end && *end != '/' && *end != '\\') {
			end++;
		}
		size_t len = (size_t)(end - start);
		if (str_eq_len_lit(start, len, "..")) {
			if (p > real_path + sizeof(BASE_PATH) - 1) {
				p--;
				while (p > real_path + sizeof(BASE_PATH) && p[-1] != '/') {
					p--;
				}
			}
		} else if (!str_eq_len_lit(start, len, ".")) {
			memcpy(p, start, len);
			p += len;
			*p++ = '/';
		}
		start = end;
	}
	if (p > real_path + sizeof(BASE_PATH) - 1) {
		p--;
	}
	*p++ = '\0';
	const char *new_real_path = realloc(real_path, p - real_path);
	if (!new_real_path) {
		free(real_path);
		return APP_ERR_INTERNAL;
	}
	return new_real_path;
}

static void app_print_cloze(FILE *file, List *plist) {
	char buffer[128];
	while (fgets(buffer, sizeof(buffer), file)) {
		char *p = buffer;
		while (*p) {
			if (*p == '[') {
				char *end = strchr(p + 1, ']');
				if (end) {
					size_t len = (size_t)(end - p - 1);
					char *answer = malloc(len + 1);
					if (answer) {
						memcpy(answer, p + 1, len);
						answer[len] = '\0';
						list_push(plist, &answer);
					}
					printf("[?]");
					p = end + 1;
					continue;
				}
			}
			putchar(*p);
			p++;
		}
	}
}

static void app_print_dir_recursive(const char *path, int depth) {
	struct _finddata_t data;
	char pattern[MAX_PATH];
	char child_path[MAX_PATH];

	strcpy(pattern, path);
	strcat(pattern, "\\*");

	intptr_t handle = _findfirst(pattern, &data);
	if (handle == -1) {
		return;
	}
	do {
		if (strcmp(data.name, ".") == 0 || strcmp(data.name, "..") == 0) {
			continue;
		}
		if (!(data.attrib & _A_SUBDIR)) {
			continue;
		}
		for (int i = 0; i < depth; i++) {
			printf("©¦   ");
		}
		printf("©¸©¤©¤ %s\n", data.name);
		strcpy(child_path, path);
		strcat(child_path, "\\");
		strcat(child_path, data.name);
		app_print_dir_recursive(child_path, depth + 1);
	} while (_findnext(handle, &data) == 0);
	_findclose(handle);
}

static int app_sync_scheduler_entries(AppState *s, sqlite3 *db) {
	// Collect files
	char **files = NULL;
	size_t file_count = 0;
	int result = app_collect_files(s->path, &files, &file_count);
	if (result) {
		return result;
	}
	if (file_count) {
		// Ensure each file has an entry in the database
		if (sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS scheduler (path TEXT PRIMARY KEY, due_date INTEGER NOT NULL, r REAL NOT NULL, s REAL NOT NULL, d REAL NOT NULL);", NULL, NULL, NULL) != SQLITE_OK) {
			result = APP_ERR_INTERNAL;
			goto app_sync_scheduler_entries_cleanup_files;
		}
		sqlite3_stmt *stmt;
		if (sqlite3_prepare_v2(db, "INSERT OR IGNORE INTO scheduler (path, due_date, r, s, d) VALUES (?, ?, ?, ?, ?);", -1, &stmt, NULL) != SQLITE_OK) {
			result = APP_ERR_INTERNAL;
			goto app_sync_scheduler_entries_cleanup_files;
		}
		for (size_t i = 0; i < file_count; i++) {
			char *path = files[i];
			printf("debug--ensure entry exists: %s\n", path);
			sqlite3_bind_text(stmt, 1, path, -1, SQLITE_TRANSIENT);
			sqlite3_bind_int64(stmt, 2, 0);
			sqlite3_bind_int64(stmt, 3, 0);
			sqlite3_bind_int64(stmt, 4, 0);
			sqlite3_bind_int64(stmt, 5, 0);
			if (sqlite3_step(stmt) != SQLITE_DONE) {
				result = APP_ERR_INTERNAL;
				goto app_sync_scheduler_entries_cleanup_stmt;
			}
			sqlite3_reset(stmt);
			sqlite3_clear_bindings(stmt);
		}
		// Free stmt
	app_sync_scheduler_entries_cleanup_stmt:
		sqlite3_finalize(stmt);
	}
	// Free collected files
app_sync_scheduler_entries_cleanup_files:
	for (size_t i = 0; i < file_count; i++) {
		free(files[i]);
	}
	free(files);
	return result;
}

int app_add(AppState *s, const char *content) {
	char path[MAX_PATH];
	do {
		sprintf(path, "%s/%08X", s->path, (unsigned int)rand());
	} while (_access(path, 0) == 0);
	FILE *file = fopen(path, "w");
	if (!file) {
		return APP_ERR_INTERNAL;
	}
	fputs(content, file);
	fclose(file);
	return 0;
}

int app_cd(AppState *s, const char *path) {
	const char *real_path = app_parse_path(s, path);
	if (!dir_exists(real_path)) {
		free(real_path);
		return APP_ERR_DIR_NOTFOUND;
	}
	if (s->path != BASE_PATH) {
		free(s->path);
	}
	s->path = real_path;
	return 0;
}

int app_create(AppState *s, const char *path) {
	if (!path) {
		return 0;
	}
	size_t root_len = strlen(s->path);
	size_t path_len = strlen(path);
	char *real_path = malloc(root_len + 1 + path_len + 1);
	if (!real_path) {
		return APP_ERR_INTERNAL;
	}
	memcpy(real_path, s->path, root_len);
	real_path[root_len] = '/';
	memcpy(real_path + root_len + 1, path, path_len);
	real_path[root_len + 1 + path_len] = '\0';
	for (char *p = real_path; *p; p++) {
		if (*p != '/' && *p != '\\') {
			continue;
		}
		char separator = *p;
		*p = '\0';
		if (*real_path && _mkdir(real_path) != 0 && errno != EEXIST) {
			free(real_path);
			return APP_ERR_INTERNAL;
		}
		*p = separator;
	}
	if (_mkdir(real_path) != 0 && errno != EEXIST) {
		free(real_path);
		return APP_ERR_INTERNAL;
	}
	free(real_path);
	return 0;
}

int app_ls(AppState *s) {
	struct _finddata_t data;
	char pattern[MAX_PATH];
	char path[MAX_PATH];

	strcpy(pattern, s->path);
	strcat(pattern, "\\*");
	intptr_t handle = _findfirst(pattern, &data);
	if (handle == -1) {
		return APP_ERR_INTERNAL;
	}
	do {
		if (strcmp(data.name, ".") == 0 || strcmp(data.name, "..") == 0) {
			continue;
		}
		strcpy(path, s->path);
		strcat(path, "\\");
		strcat(path, data.name);
		if ((data.attrib & _A_SUBDIR)) {
			printf("%s/\n", data.name);
		} else {
			FILE *file = fopen(path, "r");
			printf("%s\t", data.name);
			print_file(file);
			fclose(file);
		}
	} while (_findnext(handle, &data) == 0);
	return 0;
}

// Currently, just a simple scheduler
static int app_update_scheduler_entry(sqlite3 *db, char *path, int grade) {
	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(db, "UPDATE scheduler SET due_date = ? WHERE path = ?;", -1, &stmt, NULL ) != SQLITE_OK) {
		return APP_ERR_INTERNAL;
	}
	int interval;
	if (grade == 0) {
		interval = 0;
	} else if (grade == 1) {
		interval = 60;
	} else if (grade == 2) {
		interval = 120;
	} else {
		interval = 240;
	}
	sqlite3_bind_int64(stmt, 1, time(NULL) + interval);
	sqlite3_bind_text(stmt, 2, path, -1, SQLITE_TRANSIENT);
	if (sqlite3_step(stmt) != SQLITE_DONE) {
		sqlite3_finalize(stmt);
		return APP_ERR_INTERNAL;
	}
	sqlite3_finalize(stmt);
	return 0;
}

static int app_prompt_grade(sqlite3 *db, const char *path) {
	printf("Grade 1-4: ");
	for (;;) {
		const char *line = read_line();
		if (!line) {
			return APP_ERR_INTERNAL;
		}
		if (strlen(line) == 1 && line[0] >= '1' && line[0] <= '4') {
			int grade = line[0] - '1';
			free(line);
			if (app_update_scheduler_entry(db, path, grade)) {
				return APP_ERR_INTERNAL;
			}
			break;
		}
		free(line);
		printf("Please enter a number between 1 and 4: ");
	}
	return 0;
}

// How study works:
// Collect all flashcards
// Ensure all flashcards have entries in SQL database
// Retrieve due flashcards
// Given a question: "The mitochondria is the [powerhouse] of the [cell]."
// Output question: "The mitochondria is the [?] of the [?]."
// Answer: powerhouse, cell (each answer split by comma, trimmed for whitespace)
// Verify answer.
int app_study(AppState *s) {
	// Open SQL
	sqlite3 *db;
	if (sqlite3_open("scheduler.db", &db) != SQLITE_OK) {
		return APP_ERR_INTERNAL;
	}

	// Register SQL entries
	int result = app_sync_scheduler_entries(s, db);
	if (result) {
		goto app_study_cleanup_db;
	}

	// Get due files
	List due_files; // List<char *>
	if (list_init(&due_files, sizeof(char *), BUFFER_LIST_CAPACITY)) {
		result = APP_ERR_INTERNAL;
		goto app_study_cleanup_db;
	}
	result = app_get_due_files(s, db, &due_files);
	if (result) {
		goto app_study_cleanup_due_files;
	}

	// Begin studying
	if (due_files.length == 0) {
		printf("You are all caught up for now.\n");
	} else {
		List answers; // List<char *>
		if (list_init(&answers, sizeof(char *), BUFFER_LIST_CAPACITY)) {
			result = APP_ERR_INTERNAL;
			goto app_study_cleanup_due_files;
		}

		while (due_files.length) {
			int index = rand() % due_files.length;
			const char *path = *(char **)list_at(&due_files, index);
			FILE *file = fopen(path, "r");
			if (!file) {
				result = APP_ERR_INTERNAL;
				break;
			}
			printf("Q: ");
			app_print_cloze(file, &answers);
			fclose(file);
			putchar('\n');

			_getch();
			printf("A: ");
			for (size_t i = 0; i < answers.length; i++) {
				if (i > 0) {
					printf(", ");
				}
				printf("%s", *(char **)list_at(&answers, i));
			}
			putchar('\n');
			for (size_t i = 0; i < answers.length; i++) {
				free(*(char **)list_at(&answers, i));
			}
			list_clear(&answers, BUFFER_LIST_CAPACITY);

			result = app_prompt_grade(db, path);
			if (result) {
				goto app_study_cleanup_due_files;
			}

			list_removen(&due_files, index, 1);
		}
		list_free(&answers);
	}

	// Free due files
app_study_cleanup_due_files:
	for (size_t i = 0; i < due_files.length; i++) {
		free(*(char **)list_at(&due_files, i));
	}
	list_free(&due_files);

	// Close SQL
app_study_cleanup_db:
	sqlite3_close(db);

	return result;
}

void app_tree(const char *path) {
	printf("%s\n", path);
	app_print_dir_recursive(path, 0);
}

int app_init(AppState *s) {
	s->path = BASE_PATH;
	return 0;
}