#ifndef _SAFE_H
#define _SAFE_H

#include <stdint.h>
#include <sys/types.h>

#define ERROR 0

int safe_open(const char *filename, int mode, int perms);
int safe_write(int fd, void *buffer, size_t size);
int safe_read(int fd, void *buffer, size_t size);
void *safe_malloc(uint64_t size);
void *safe_realloc(void *prev, uint64_t size);
void *safe_calloc(uint64_t num, uint64_t size);

#endif
