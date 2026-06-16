#include "include/mmap.h"
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>

static int odin_to_posix_prot(int prot) {
    int posix_prot = 0;

    if (prot & ODIN_PROT_READ)  posix_prot |= PROT_READ;
    if (prot & ODIN_PROT_WRITE) posix_prot |= PROT_WRITE;
    if (prot & ODIN_PROT_EXEC)  posix_prot |= PROT_EXEC;
    return posix_prot;
}

static int odin_to_posix_map(int flags) {
    int posix_flags = 0;

    if (flags & ODIN_MAP_SHARED)  posix_flags |= MAP_SHARED;
    if (flags & ODIN_MAP_PRIVATE) posix_flags |= MAP_PRIVATE;
    if (flags & ODIN_MAP_ANON) {
#if defined(MAP_ANON)
        posix_flags |= MAP_ANON;
#elif defined(MAP_ANONYMOUS)
        posix_flags |= MAP_ANONYMOUS;
#endif
    }
    return posix_flags;
}

void* sys_mmap(void *addr, size_t length, int prot, int flags, int fd, long offset) {
    void *result;
    int posix_prot;
    int posix_flags;

    posix_prot = odin_to_posix_prot(prot);
    posix_flags = odin_to_posix_map(flags);

    result = mmap(addr, length, posix_prot, posix_flags, fd, (off_t)offset);
    if (result == MAP_FAILED) {
        sys_seterr_posix();
        return SYS_MAP_FAILED;
    }
    return result;
}

int sys_mprotect(void *addr, size_t len, int prot) {
    int posix_prot;
    int result;

    posix_prot = odin_to_posix_prot(prot);
    result = mprotect(addr, len, posix_prot);
    if (result != 0) {
        return sys_seterr_posix();
    }
    return 0;
}

int sys_munmap(void *addr, size_t length) {
    int result;

    result = munmap(addr, length);
    if (result != 0) {
        return sys_seterr_posix();
    }
    return 0;
}
