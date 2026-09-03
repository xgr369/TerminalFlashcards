#include "list.h"
#include <string.h>

static int list_resize(List *pv, size_t size) {
	pv->size = size;
	void *ptr = realloc(pv->data, pv->size * pv->elemSize);
	if (!ptr)
		return 1;
	pv->data = ptr;
	return 0;
}

int list_init(List *pv, size_t elemSize, size_t size) {
	if (!pv || elemSize == 0)
		return 1;
	pv->data = malloc(size * elemSize);
	if (!pv->data)
		return 1;
	pv->length = 0;
	pv->size = size;
	pv->elemSize = elemSize;
	return 0;
}

int list_clear(List *pv, size_t size) {
	if (!pv)
		return 1;
	if (list_resize(pv, size))
		return 1;
	pv->length = 0;
	return 0;
}

int list_free(List *pv) {
	if (!pv || !pv->data)
		return 1;
	free(pv->data);
	pv->data = NULL;
	return 0;
}

void *list_at(List *pv, size_t index) {
	if (!pv || index < 0 || index >= pv->length)
		return NULL;
	return pv->data + pv->elemSize * index;
}

int list_get(List *pv, size_t index, void *dst) {
	if (!pv || !dst || index < 0 || index >= pv->length)
		return 1;
	memcpy(dst, pv->data + index * pv->elemSize, pv->elemSize);
	return 0;
}

int list_pop(List *pv, void *dst) {
	if (!pv || pv->length == 0)
		return 1;
	if (dst)
		memcpy(dst, pv->data + (pv->length - 1) * pv->elemSize, pv->elemSize);
	pv->length--;
	return 0;
}

int list_popn(List *pv, size_t n) {
	if (!pv || n < 0 || pv->length < n)
		return 1;
	pv->length -= n;
	return 0;
}

int list_push(List *pv, const void *src) {
	if (!pv || !src)
		return 1;
	if (pv->length * pv->elemSize >= pv->size)
		if (list_resize(pv, pv->size ? pv->size * 2 : 1))
			return 1;
	memcpy(pv->data + pv->length * pv->elemSize, src, pv->elemSize);
	pv->length++;
	return 0;
}

int list_removen(List *pv, size_t index, size_t n) {
	if (!pv || index > pv->length || n > pv->length - index)
		return 1;
	size_t tailSize = pv->length - index - n;
	memmove(pv->data + index * pv->elemSize, pv->data + (index + n) * pv->elemSize, tailSize * pv->elemSize);
	pv->length -= n;
	return 0;
}

int list_set(List *pv, size_t index, void *src) {
	if (!pv || !src || index >= pv->length)
		return 1;
	memcpy(pv->data + index * pv->elemSize, src, pv->elemSize);
	return 0;
}