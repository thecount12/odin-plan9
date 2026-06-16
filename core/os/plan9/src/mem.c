#include <u.h>
#include <libc.h>
#include "mem.h"

void *
sys_malloc(ulong size)
{
	if(size == 0)
		return nil;
	return malloc(size);
}

void *
sys_calloc(ulong nmemb, ulong size)
{
	if(nmemb == 0 || size == 0)
		return nil;
	return calloc(nmemb, size);
}

void *
sys_realloc(void *ptr, ulong size)
{
	if(ptr == nil)
		return sys_malloc(size);
	if(size == 0) {
		sys_free(ptr);
		return nil;
	}
	return realloc(ptr, size);
}

void
sys_free(void *ptr)
{
	if(ptr != nil)
		free(ptr);
}

void *
sys_memcpy(void *dest, void *src, ulong n)
{
	return memmove(dest, src, n);
}

void *
sys_memset(void *s, int c, ulong n)
{
	return memset(s, c, n);
}

int
sys_memcmp(void *s1, void *s2, ulong n)
{
	return memcmp(s1, s2, n);
}

long
sys_strlen(char *s)
{
	if(s == nil)
		return 0;
	return (long)strlen(s);
}
