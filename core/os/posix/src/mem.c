#include "include/mem.h"
#include <stdlib.h>
#include <string.h>

void *sys_malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    return malloc(size);
}

void *sys_calloc(size_t nmemb, size_t size) {
    if (nmemb == 0 || size == 0) {
        return NULL;
    }
    return calloc(nmemb, size);
}

void *sys_realloc(void *ptr, size_t size) {
    if (ptr == NULL) {
        return sys_malloc(size);
    }
    if (size == 0) {
        sys_free(ptr);
        return NULL;
    }
    return realloc(ptr, size);
}

void sys_free(void *ptr) {
    if (ptr != NULL) {
        free(ptr);
    }
}

void *sys_memcpy(void *dest, const void *src, size_t n) {
    return memcpy(dest, src, n);
}

void *sys_memset(void *s, int c, size_t n) {
    return memset(s, c, n);
}

int sys_memcmp(const void *s1, const void *s2, size_t n) {
    return memcmp(s1, s2, n);
}

long sys_strlen(const char *s) {
    if (s == NULL) {
        return 0;
    }
    return (long)strlen(s);
}
