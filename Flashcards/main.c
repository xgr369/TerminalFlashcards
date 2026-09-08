#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core.h"
#include "common.h"

#define check_command(str, l) (((str)[sizeof(l) - 1] == '\0' || (str)[sizeof(l) - 1] == ' ') && strncmp((str), (l), sizeof(l) - 1) == 0)
#define get_argument(str, l) ((str)[sizeof(l) - 1] == '\0' ? NULL : (str) + sizeof(l))

int exec_line(AppState *s, char *line) {
    if (line[0] == '\0') {
        return 0;
	}
    if (check_command(line, "exit")) {
        return 1;
    }
    if (check_command(line, "help")) {
        printf("Available commands:\n");
        printf("%-14s%s\n", "add", "Adds a flashcard to the current set.");
        printf("%-14s%s\n", "cd", "Changes the current set.");
        printf("%-14s%s\n", "ls", "Lists the flashcards in the current set.");
        printf("%-14s%s\n", "create", "Creates a set.");
        printf("%-14s%s\n", "study", "Begins flashcard study.");
        printf("%-14s%s\n", "tree", "Lists the sets.");
        printf("%-14s%s\n", "exit", "Quits the program.");
	} else if (check_command(line, "cd")) {
        char *arg = get_argument(line, "cd");
        if (!arg) {
            goto exec_line_err_noarg;
        }
        int result = app_cd(s, arg);
        if (result == APP_ERR_INTERNAL) {
            goto exec_line_err_internal;
        } else if (result == APP_ERR_DIR_NOTFOUND) {
            goto exec_line_err_dir_notfound;
        }
    } else if (check_command(line, "ls")) {
        int result = app_ls(s);
        if (result == APP_ERR_INTERNAL) {
            goto exec_line_err_internal;
        } else if (result == APP_ERR_DIR_NOTFOUND) {
            goto exec_line_err_dir_notfound;
        }
    } else if (check_command(line, "add")) {
        char *arg = get_argument(line, "add");
        if (!arg) {
            goto exec_line_err_noarg;
        }
        if (app_add(s, arg)) {
            goto exec_line_err_internal;
        }
    } else if (check_command(line, "study")) {
        if (app_study(s)) {
            goto exec_line_err_internal;
        }
    } else if (check_command(line, "create")) {
        char *arg = get_argument(line, "create");
        if (!arg) {
            goto exec_line_err_noarg;
        }
        if (app_create(s, arg)) {
            goto exec_line_err_internal;
        }
        printf("Created set '%s'.\n", arg);
    } else if (check_command(line, "tree")) {
        app_tree(s->path);
    } else {
        printf("Unrecognized command: %s\n", line);
    }
    return 0;
exec_line_err_noarg:
    printf("Missing argument for command '%s'\n", line);
	return 0;
exec_line_err_dir_notfound:
    printf("Directory not found.\n");
    return 0;
exec_line_err_internal:
    printf("Internal error.\n");
    return 0;
}

int main(int argc, char **argv) {
    printf("Flashcard Application\n");
    AppState state;
    app_init(&state);
    for (;;) {
        printf("%s>", state.path);
		char *line = read_line();
        if (line == NULL) {
            continue;
		}
        int result = exec_line(&state, line);
        free(line);
        if (result == 1) {
            break;
		}
    }
}