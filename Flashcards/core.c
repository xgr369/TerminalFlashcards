#define _CRT_SECURE_NO_WARNINGS
#include "core.h"
#include "common.h"
#include "list.h"
#include <direct.h>
#include <errno.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define MAX_PATH 256
#define BASE_PATH "flashcards"
#define BUFFER_LIST_CAPACITY 2

// Compares if the string is equal to the specified span
static int streqspan(const char *str, const char *start, size_t n) {
	char *p1 = str, *p2 = start;
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

static void app_evaluate_answer(const char *line, List *plist) {
	char *p = line;
	int i = 0;
	for (;;) {
		while (*p && isspace(*p)) {
			p++;
		}
		char *start = p;
		while (*p && *p != ',' && !isspace(*p)) {
			p++;
		}
		size_t n = p - start;
		while (*p && isspace(*p)) {
			p++;
		}
		char *answer;
		list_get(plist, i, &answer);
		if (streqspan(answer, start, n)) {
			printf("Correct!\n");
		} else {
			printf("Wrong.\n");
		}
		i++;
		if (i >= plist->length) {
			break;
		}
		if (!*p) {
			break;
		}
		p++;
	}
	if (i < plist->length) {
		printf(plist->length - i == 1 ? "Missing answer.\n" : "Missing answers.\n");
	}
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
	putchar('\n');
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
		if (strcmp(data.name, ".") == 0 ||
			strcmp(data.name, "..") == 0) {
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

int app_add(AppState *s, const char *content) {
	char path[MAX_PATH];
	do {
		sprintf(path, "%s\\%08X.txt", s->path, (unsigned int)rand());
	} while (_access(path, 0) == 0);
	FILE *file = fopen(path, "w");
	if (!file) {
		return -1;
	}
	fputs(content, file);
	fclose(file);
	return 0;
}

// todo: actually navigate while parsing the path
int app_cd(AppState *s, const char *path) {
	if (path[0] == '\0') {
		s->path = BASE_PATH;
		return 0;
	}
	char *new_path = malloc(sizeof(BASE_PATH) + strlen(path) + 1);
	if (!new_path) {
		return 1;
	}
	strcpy(new_path, BASE_PATH);
	new_path[sizeof(BASE_PATH) - 1] = '/';
	strcpy(new_path + sizeof(BASE_PATH), path);
	s->path = new_path;
	return 0;
}

int app_create(AppState *s, const char *name) {
	if (!name) {
		return 0;
	}
	size_t root_len = strlen(s->path);
	size_t name_len = strlen(name);
	char *path = malloc(root_len + 1 + name_len + 1);
	if (!path) {
		return -1;
	}
	memcpy(path, s->path, root_len);
	path[root_len] = '/';
	memcpy(path + root_len + 1, name, name_len);
	path[root_len + 1 + name_len] = '\0';
	for (char *p = path; *p; p++) {
		if (*p != '/' && *p != '\\') {
			continue;
		}
		char separator = *p;
		*p = '\0';
		if (*path && _mkdir(path) != 0 && errno != EEXIST) {
			free(path);
			return -1;
		}
		*p = separator;
	}
	if (_mkdir(path) != 0 && errno != EEXIST) {
		free(path);
		return -1;
	}
	free(path);
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
		return 0;
	}
	do {
		if (strcmp(data.name, ".") == 0 || strcmp(data.name, "..") == 0) {
			continue;
		}
		strcpy(path, s->path);
		strcat(path, "\\");
		strcat(path, data.name);
		if (!(data.attrib & _A_SUBDIR)) {
			FILE *file = fopen(path, "r");
			printf("Q: ");
			print_file(file);
			fclose(file);
		}
	} while (_findnext(handle, &data) == 0);
}

static int collect_files(const char *dir, char ***files, size_t *count) {
	struct _finddata_t data;
	char pattern[MAX_PATH];
	char path[MAX_PATH];

	strcpy(pattern, dir);
	strcat(pattern, "\\*");
	intptr_t handle = _findfirst(pattern, &data);
	if (handle == -1) {
		return 0;
	}
	do {
		if (strcmp(data.name, ".") == 0 || strcmp(data.name, "..") == 0) {
			continue;
		}
		strcpy(path, dir);
		strcat(path, "\\");
		strcat(path, data.name);
		if (data.attrib & _A_SUBDIR) {
			if (collect_files(path, files, count)) {
				_findclose(handle);
				return 1;
			}
		} else {
			char *file = malloc(strlen(path) + 1);
			if (!file) {
				_findclose(handle);
				return 1;
			}
			strcpy(file, path);
			char **new_files = realloc(*files, (*count + 1) * sizeof(char *));
			if (!new_files) {
				free(file);
				_findclose(handle);
				return 1;
			}
			*files = new_files;
			(*files)[*count] = file;
			(*count)++;
		}
	} while (_findnext(handle, &data) == 0);
	_findclose(handle);
	return 0;
}

// How study works:
// Given a question: "The mitochondria is the [powerhouse] of the [cell]."
// Output question: "The mitochondria is the [?] of the [?]."
// Answer: powerhouse, cell (each answer split by comma, trimmed for whitespace)
// Verify answer.
int app_study(AppState *s) {
	char **files = NULL;
	size_t file_count = 0;
	if (collect_files(s->path, &files, &file_count)) {
		return 1;
	}
	if (file_count == 0) {
		free(files);
		return 0;
	}
	int result = 0;
	List answers; // List<char *>
	if (list_init(&answers, sizeof(char *), BUFFER_LIST_CAPACITY)) {
		result = 1;
	} else {
		for (;;) {
			char *path = files[rand() % file_count];
			FILE *file = fopen(path, "r");
			if (!file) {
				result = 1;
				break;
			}
			printf("Q: ");
			app_print_cloze(file, &answers);
			/*for (int i = 0; i < answers.length; i++) {
				char *answer;
				list_get(&answers, i, &answer);
				printf("%s, ", answer);
			}
			putchar('\n');*/ // debug -- show the answers
			printf("A: ");
			char *line = read_line();
			if (line == NULL) {
				result = 1;
				break;
			}
			if (line[0] == '\0') {
				free(line);
				break;
			}
			app_evaluate_answer(line, &answers);
			free(line);
			fclose(file);
			for (int i = 0; i < answers.length; i++) {
				char *answer;
				list_get(&answers, i, &answer);
				free(answer);
			}
			list_clear(&answers, BUFFER_LIST_CAPACITY);
		}
	}
	list_free(&answers);
	for (size_t i = 0; i < file_count; i++) {
		free(files[i]);
	}
	free(files);
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