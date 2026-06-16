#ifndef MMAP_H
#define MMAP_H

#include "sysdeps.h"

/* Protection flags (portable Odin names; mapped to POSIX in mmap.c) */
enum {
    ODIN_PROT_NONE  = 0,
    ODIN_PROT_READ  = 1,
    ODIN_PROT_WRITE = 2,
    ODIN_PROT_EXEC  = 4
};

#define PROT_NONE  ODIN_PROT_NONE
#define PROT_READ  ODIN_PROT_READ
#define PROT_WRITE ODIN_PROT_WRITE
#define PROT_EXEC  ODIN_PROT_EXEC

/* Map flags (portable Odin names; mapped to POSIX in mmap.c) */
enum {
    ODIN_MAP_SHARED  = 0x001,
    ODIN_MAP_PRIVATE = 0x002,
    ODIN_MAP_ANON    = 0x004
};

#define MAP_SHARED  ODIN_MAP_SHARED
#define MAP_PRIVATE ODIN_MAP_PRIVATE
#define MAP_ANON    ODIN_MAP_ANON

#define SYS_MAP_FAILED ((void *)-1)

void* sys_mmap(void *addr, size_t length, int prot, int flags, int fd, long offset);
int sys_mprotect(void *addr, size_t len, int prot);
int sys_munmap(void *addr, size_t length);

#endif
