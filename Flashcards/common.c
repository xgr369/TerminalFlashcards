#include "common.h"
#include <stdlib.h>

char *read_line(void) {
    size_t capacity = 64;
    size_t length = 0;
    char *buffer = malloc(capacity);
    if (!buffer) {
        return NULL;
    }
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {
        if (length + 1 >= capacity) {
            capacity *= 2;
            char *new_buffer = realloc(buffer, capacity);
            if (!new_buffer) {
                free(buffer);
                return NULL;
            }
            buffer = new_buffer;
        }
        buffer[length++] = (char)c;
    }
    buffer[length] = '\0';
    return buffer;
}

int print_file(FILE *file) {
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), file)) {
        printf("%s", buffer);
    }
    putchar('\n');
}