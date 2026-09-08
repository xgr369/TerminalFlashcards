#ifndef CORE_H
#define CORE_H

#define APP_ERR_INTERNAL 1
#define APP_ERR_DIR_NOTFOUND 2

typedef struct {
	char *path;
} AppState;

int app_add(AppState *s, const char *content);
int app_cd(AppState *s, const char *path);
int app_create(AppState *s, const char *path);
int app_ls(AppState *s);
int app_study(AppState *s);
void app_tree(const char *path);

int app_init(AppState *s);
#endif // CORE_H