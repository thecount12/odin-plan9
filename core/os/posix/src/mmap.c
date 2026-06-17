#include "include/mmap.h"
#include "include/sysdeps.h"
#include <sys/mman.h>
#include <errno.h>

#if defined(__APPLE__) && !defined(MAP_ANONYMOUS)
#define MAP_ANONYMOUS MAP_ANON
#endif

static int
odin_to_posix_prot(int prot)
{
    int posix_prot;

    posix_prot = 0;
    if (prot & ODIN_PROT_READ) {
        posix_prot |= PROT_READ;
    }
    if (prot & ODIN_PROT_WRITE) {
        posix_prot |= PROT_WRITE;
    }
    if (prot & ODIN_PROT_EXEC) {
        posix_prot |= PROT_EXEC;
    }
    return posix_prot;
}

static int
odin_to_posix_flags(int flags)
{
    int posix_flags;

    posix_flags = 0;
    if (flags & ODIN_MAP_SHARED) {
        posix_flags |= MAP_SHARED;
    }
    if (flags & ODIN_MAP_PRIVATE) {
        posix_flags |= MAP_PRIVATE;
    }
    if (flags & ODIN_MAP_ANONYMOUS) {
        posix_flags |= MAP_ANONYMOUS;
    }
    if (flags & ODIN_MAP_FIXED) {
        posix_flags |= MAP_FIXED;
    }
    return posix_flags;
}

void *
sys_mmap(void *addr, ulong length, int prot, int flags, fd_t fd, ulong offset)
{
    void *result;
    int posix_prot;
    int posix_flags;

    if (length == 0) {
        sys_seterr(ERR_IO);
        return NULL;
    }

    posix_prot = odin_to_posix_prot(prot);
    posix_flags = odin_to_posix_flags(flags);

    result = mmap(addr, (size_t)length, posix_prot, posix_flags, (int)fd, (off_t)offset);
    if (result == MAP_FAILED) {
        sys_seterr_posix();
        return NULL;
    }
    return result;
}

int
sys_munmap(void *addr, ulong length)
{
    if (addr == NULL || length == 0) {
        sys_seterr(ERR_IO);
        return -1;
    }
    if (munmap(addr, (size_t)length) < 0) {
        return sys_seterr_posix();
    }
    return 0;
}

int
sys_mprotect(void *addr, ulong length, int prot)
{
    int posix_prot;

    if (addr == NULL || length == 0) {
        sys_seterr(ERR_IO);
        return -1;
    }

    posix_prot = odin_to_posix_prot(prot);
    if (mprotect(addr, (size_t)length, posix_prot) < 0) {
        return sys_seterr_posix();
    }
    return 0;
}
