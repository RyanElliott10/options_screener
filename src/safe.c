#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>

#include "../include/safe.h"

/* Safe wrapper for open() */
int safe_open(const char *filename, int mode, int perms) {
   int fd;

   if ((fd = open(filename, mode, perms)) < ERROR) {
      perror(filename);
      exit(1);
   }

   return fd;
}

/* Safe wrapper for read() */
int safe_read(int fd, void *buffer, size_t size) {
   int ret;

   if ((ret = read(fd, buffer, size)) < 0) {
      perror("read error\n");
      exit(1);
   }

   return ret;
}

/* Safe wrapper for write() */
int safe_write(int fd, void *buffer, size_t size) {
   int ret;

   if ((ret = write(fd, buffer, size)) != size) {
      perror("read error\n");
      exit(1);
   }

   return ret;
}

/* Safe wrapper for malloc() */
void *safe_malloc(uint64_t size) {
   void *new;

   if((new = malloc(size)) == NULL) {
      fprintf(stderr, "malloc error\n");
      exit(1);
   }

   return new;
}

/* Safe wrapper for realloc() */
void *safe_realloc(void *prev, uint64_t size) {
   void *new;

   if((new = realloc(prev, size)) == NULL) {
      fprintf(stderr, "realloc error\n");
      exit(1);
   }

   return new;
}

/* Safe wrapper for calloc() */
void *safe_calloc(uint64_t num, uint64_t size) {
   void *new;

   if((new = calloc(num, size)) == NULL) {
      fprintf(stderr, "calloc error\n");
      exit(1);
   }

   return new;
}
