#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>

#define str_eq_len_lit(str, len, l) ((len) == sizeof(l) - 1 && strncmp((str), (l), sizeof(l) - 1) == 0)
#if false
#define debug(fmt, ...) do { \
	printf("debug -- " fmt, __VA_ARGS__); \
} while (0)
#else
#define debug(fmt, ...) 
#endif

char *read_line(void);
int print_file(FILE *file);
int dir_exists(const char *path);
int file_exists(const char *path);

#endif // !COMMON_H