#ifndef MMAP_H
#define MMAP_H

#include "sysdeps.h"

/* Protection flags (Odin values; mapped to PROT_* in mmap.c) */
enum {
    ODIN_PROT_NONE  = 0,
    ODIN_PROT_READ  = 1,
    ODIN_PROT_WRITE = 2,
    ODIN_PROT_EXEC  = 4
};

/* Mapping flags (Odin values; mapped to MAP_* in mmap.c) */
enum {
    ODIN_MAP_SHARED    = 0x01,
    ODIN_MAP_PRIVATE   = 0x02,
    ODIN_MAP_ANONYMOUS = 0x10,
    ODIN_MAP_FIXED     = 0x100
};

void *sys_mmap(void *addr, ulong length, int prot, int flags, fd_t fd, ulong offset);
int sys_munmap(void *addr, ulong length);
int sys_mprotect(void *addr, ulong length, int prot);

#endif
