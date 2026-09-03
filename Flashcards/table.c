#include "table.h"
#include <string.h>
#include <stdint.h>

#define TABLE_HASH_SEED 5381u
#define TABLE_MAX_LOAD_FACTOR 0.75

static uint64_t hash(const char *key) {
    uint64_t pt = TABLE_HASH_SEED;
    while (*key) {
        pt = ((pt << 5) + pt) + (unsigned char)*key++;
    }
    return pt;
}

static TableNode *tablenode_find(TableNode *node, const char *key) {
    while (node) {
        if (strcmp(node->key, key) == 0)
            return node;
        node = node->next;
    }
    return NULL;
}
static TableNode *tablenode_new( const char *key, const void *src, size_t elemSize) {
    size_t size = sizeof(TableNode) + elemSize;
    TableNode *node = malloc(size);
    if (!node)
        return NULL;
    node->key = _strdup(key);
    if (!node->key) {
        free(node);
        return NULL;
    }
    node->next = NULL;
    memcpy(node->value, src, elemSize);
    return node;
}

static void tablenode_free(TableNode *node) {
    while (node) {
        TableNode *next = node->next;
        free(node->key);
        free(node);
        node = next;
    }
}

static int rehash(Table *pt, size_t newSize) {
    TableNode **newData = calloc(newSize, sizeof(*newData));
    if (!newData)
        return 1;
    for (size_t i = 0; i < pt->size; i++) {
        TableNode *node = pt->data[i];
        while (node) {
            TableNode *next = node->next;
            size_t index = hash(node->key) % newSize;
            node->next = newData[index];
            newData[index] = node;
            node = next;
        }
    }
    free(pt->data);
    pt->data = newData;
    pt->size = newSize;
    return 0;
}

int table_init(Table *pt, size_t elemSize, size_t size) {
    if (!pt || elemSize == 0 || size == 0)
        return 1;
    pt->data = calloc(size, sizeof(*pt->data));
    if (!pt->data)
        return 1;
    pt->size = size;
    pt->elemSize = elemSize;
    pt->count = 0;
    return 0;
}

int table_clear(Table *pt, size_t size) {
    if (!pt || !pt->data || size == 0)
        return 1;
    TableNode **data = calloc(size, sizeof(*data));
    if (!data)
        return 1;
    for (size_t i = 0; i < pt->size; i++)
        tablenode_free(pt->data[i]);
    free(pt->data);
    pt->data = data;
    pt->size = size;
    pt->count = 0;
    return 0;
}

int table_free(Table *pt) {
    if (!pt)
        return 1;
    for (size_t i = 0; i < pt->size; i++)
        tablenode_free(pt->data[i]);
    free(pt->data);
    pt->data = NULL;
    pt->size = 0;
    pt->elemSize = 0;
    pt->count = 0;
    return 0;
}

void *table_at(Table *pt, const char *key) {
    if (!pt || !pt->data || !key || pt->size == 0)
        return NULL;
    size_t index = hash(key) % pt->size;
    TableNode *node = tablenode_find(pt->data[index], key);
    return node ? node->value : NULL;
}

int table_containskey(Table *pt, const char *key) {
    if (!pt || !pt->data || !key || pt->size == 0)
        return 1;
    size_t index = hash(key) % pt->size;
    return tablenode_find(pt->data[index], key) == NULL;
}

int table_get(Table *pt, const char *key, void *dst) {
    if (!pt || !pt->data || !key || !dst || pt->size == 0)
        return 1;
    size_t index = hash(key) % pt->size;
    TableNode *node = tablenode_find(pt->data[index], key);
    if (!node)
        return 1;
    memcpy(dst, node->value, pt->elemSize);
    return 0;
}

int table_put(Table *pt, const char *key, const void *src) {
    if (!pt || !pt->data || !key || !src || pt->size == 0)
        return 1;
    size_t index = hash(key) % pt->size;
    TableNode *node = tablenode_find(pt->data[index], key);
    if (node) {
        memcpy(node->value, src, pt->elemSize);
        return 0;
    }
    if ((double)(pt->count + 1) / (double)pt->size > TABLE_MAX_LOAD_FACTOR) {
        if (pt->size > SIZE_MAX / 2)
            return 1;
        if (rehash(pt, pt->size * 2))
            return 1;
        index = hash(key) % pt->size;
    }
    node = tablenode_new(key, src, pt->elemSize);
    if (!node)
        return 1;
    node->next = pt->data[index];
    pt->data[index] = node;
    pt->count++;
    return 0;
}

int table_remove(Table *pt, const char *key, void *dst) {
    if (!pt || !pt->data || !key || pt->size == 0)
        return 1;
    size_t index = hash(key) % pt->size;
    TableNode **link = &pt->data[index];
    while (*link) {
        TableNode *node = *link;
        if (strcmp(node->key, key) == 0) {
            if (dst)
                memcpy(dst, node->value, pt->elemSize);
            *link = node->next;
            free(node->key);
            free(node);
            pt->count--;
            return 0;
        }
        link = &node->next;
    }
    return 1;
}