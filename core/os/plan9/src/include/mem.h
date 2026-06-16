#ifndef MEM_H
#define MEM_H

#include "sysdeps.h"

void *sys_malloc(ulong size);
void *sys_calloc(ulong nmemb, ulong size);
void *sys_realloc(void *ptr, ulong size);
void sys_free(void *ptr);
void *sys_memcpy(void *dest, void *src, ulong n);
void *sys_memset(void *s, int c, ulong n);
int sys_memcmp(void *s1, void *s2, ulong n);
long sys_strlen(char *s);

#endif
