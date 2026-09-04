#include "list.h"
#include <string.h>

static int list_resize(List *pl, size_t size) {
	pl->size = size;
	void *ptr = realloc(pl->data, pl->size * pl->elemSize);
	if (!ptr)
		return 1;
	pl->data = ptr;
	return 0;
}

int list_init(List *pl, size_t elemSize, size_t size) {
	if (!pl || elemSize == 0)
		return 1;
	pl->data = malloc(size * elemSize);
	if (!pl->data)
		return 1;
	pl->length = 0;
	pl->size = size;
	pl->elemSize = elemSize;
	return 0;
}

int list_clear(List *pl, size_t size) {
	if (!pl)
		return 1;
	if (list_resize(pl, size))
		return 1;
	pl->length = 0;
	return 0;
}

int list_free(List *pl) {
	if (!pl || !pl->data)
		return 1;
	free(pl->data);
	pl->data = NULL;
	return 0;
}

void *list_at(List *pl, size_t index) {
	if (!pl || index < 0 || index >= pl->length)
		return NULL;
	return pl->data + pl->elemSize * index;
}

int list_get(List *pl, size_t index, void *dst) {
	if (!pl || !dst || index < 0 || index >= pl->length)
		return 1;
	memcpy(dst, pl->data + index * pl->elemSize, pl->elemSize);
	return 0;
}

int list_pop(List *pl, void *dst) {
	if (!pl || pl->length == 0)
		return 1;
	if (dst)
		memcpy(dst, pl->data + (pl->length - 1) * pl->elemSize, pl->elemSize);
	pl->length--;
	return 0;
}

int list_popn(List *pl, size_t n) {
	if (!pl || n < 0 || pl->length < n)
		return 1;
	pl->length -= n;
	return 0;
}

int list_push(List *pl, const void *src) {
	if (!pl || !src)
		return 1;
	if (pl->length * pl->elemSize >= pl->size)
		if (list_resize(pl, pl->size ? pl->size * 2 : 1))
			return 1;
	memcpy(pl->data + pl->length * pl->elemSize, src, pl->elemSize);
	pl->length++;
	return 0;
}

int list_removen(List *pl, size_t index, size_t n) {
	if (!pl || index > pl->length || n > pl->length - index)
		return 1;
	size_t tailSize = pl->length - index - n;
	memmove(pl->data + index * pl->elemSize, pl->data + (index + n) * pl->elemSize, tailSize * pl->elemSize);
	pl->length -= n;
	return 0;
}

int list_set(List *pl, size_t index, void *src) {
	if (!pl || !src || index >= pl->length)
		return 1;
	memcpy(pl->data + index * pl->elemSize, src, pl->elemSize);
	return 0;
}