#ifndef CHARLIST_H
#define CHARLIST_H

#include <stdlib.h>

typedef struct {
	char* data;
	size_t length;
	size_t size;
} CharList;

int charlist_init(CharList *pcv, size_t size);
int charlist_free(CharList *pcv);
int charlist_get(CharList *pcv, size_t index, char *dst);
int charlist_pop(CharList *pcv, char *dst);
int charlist_push(CharList *pcv, char value);
int charlist_pusharray(CharList *pcv, const char *src, size_t len);
int charlist_setarray(CharList *pcv, size_t index, const char *src, size_t len);

#endif // CHARLIST_H