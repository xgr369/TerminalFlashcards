#ifndef TABLE_H
#define TABLE_H

#include <stdlib.h>

struct TableNode {
	char *key;
	struct TableNode *next;
	char value[];
};
typedef struct TableNode TableNode;

typedef struct {
	TableNode **data;
	size_t elemSize;
	size_t size;
	size_t count;
} Table;

int table_init(Table *pt, size_t elemSize, size_t size);
int table_clear(Table *pt, size_t size);
int table_free(Table *pt);
void *table_at(Table *pt, const char *key);
int table_containskey(Table *pt, const char *key);
int table_get(Table *pt, const char *key, void *dst);
int table_put(Table *pt, const char *key, const void *src);
int table_remove(Table *pt, const char *key, void *dst);

#endif // TABLE_H