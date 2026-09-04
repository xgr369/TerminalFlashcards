#ifndef LIST_H
#define LIST_H

#include <stdlib.h>

typedef struct {
	char *data;
	size_t elemSize;
	size_t length;
	size_t size;
} List;

int list_init(List *pl, size_t elemSize, size_t size);
int list_clear(List *pl, size_t size);
int list_free(List *pl);
void *list_at(List *pl, size_t index);
int list_get(List *pl, size_t index, void *dst);
int list_pop(List *pl, void *dst);
int list_popn(List *pl, size_t n);
int list_push(List *pl, const void *src);
int list_removen(List *pl, size_t index, size_t n);
int list_set(List *pl, size_t index, void *src);

#endif // LIST_H