#include "charlist.h"
#include <string.h>

static int charlist_resize(CharList *pcv, size_t addLen) {
	while (pcv->length + addLen > pcv->size)
		pcv->size = pcv->size ? pcv->size * 2 : 1;
	void* ptr = realloc(pcv->data, pcv->size);
	if (!ptr)
		return 1;
	pcv->data = ptr;
	return 0;
}

int charlist_init(CharList *pcv, size_t size) {
	if (!pcv)
		return 1;
	pcv->data = malloc(size);
	if (!pcv->data)
		return 1;
	pcv->length = 0;
	pcv->size = size;
	return 0;
}

int charlist_free(CharList *pcv) {
	if (!pcv || !pcv->data)
		return 1;
	free(pcv->data);
	pcv->data = NULL;
	return 0;
}

int charlist_get(CharList *pcv, size_t index, char *dst) {
	if (!pcv || !dst || index < 0 || index >= pcv->length)
		return 1;
	memcpy(dst, pcv->data + index, 1);
	return 0;
}

int charlist_pop(CharList *pcv, char *dst) {
	if (!pcv || !dst || pcv->length == 0)
		return 1;
	memcpy(dst, pcv->data + pcv->length - 1, 1);
	pcv->length--;
	return 0;
}

int charlist_push(CharList *pcv, char value) {
	if (!pcv)
		return 1;
	if (pcv->length >= pcv->size)
		if (charlist_resize(pcv, 1))
			return 1;
	*(pcv->data + pcv->length) = value;
	pcv->length++;
	return 0;
}

int charlist_pusharray(CharList *pcv, const char *src, size_t len) {
	if (!pcv || !src)
		return 1;
	if (pcv->length + len > pcv->size)
		if (charlist_resize(pcv, len))
			return 1;
	memcpy(pcv->data + pcv->length, src, len);
	pcv->length += len;
	return 0;
}

int charlist_setarray(CharList *pcv, size_t index, const char *src, size_t len) {
	if (!pcv || !src)
		return 1;
	if (index + len > pcv->size)
		if (charlist_resize(pcv, len))
			return 1;
	memcpy(pcv->data + index, src, len);
	return 0;
}