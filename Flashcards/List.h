#ifndef LIST_H
#define LIST_H

#include <stdlib.h>

typedef struct {
	char *data;
	size_t elemSize;
	size_t length;
	size_t size;
} List;

int list_init(List *pv, size_t elemSize, size_t size);
int list_clear(List *pv, size_t size);
int list_free(List *pv);
void *list_at(List *pv, size_t index);
int list_get(List *pv, size_t index, void *dst);
int list_pop(List *pv, void *dst);
int list_popn(List *pv, size_t n);
int list_push(List *pv, const void *src);
int list_removen(List *pv, size_t index, size_t n);
int list_set(List *pv, size_t index, void *src);

#endif // LIST_H