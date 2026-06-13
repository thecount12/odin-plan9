#ifndef MEM_H
#define MEM_H

#include "sysdeps.h"

void *sys_malloc(size_t size);
void *sys_calloc(size_t nmemb, size_t size);
void *sys_realloc(void *ptr, size_t size);
void sys_free(void *ptr);
void *sys_memcpy(void *dest, const void *src, size_t n);
void *sys_memset(void *s, int c, size_t n);
int sys_memcmp(const void *s1, const void *s2, size_t n);
long sys_strlen(const char *s);

#endif
