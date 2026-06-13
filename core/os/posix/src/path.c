#include "include/path.h"
#include <unistd.h>
#include <errno.h>

int sys_getcwd(char *buf, long size) {
    char *result;

    if (buf == NULL || size <= 0) {
        sys_seterr(ERR_IO);
        return -1;
    }

    result = getcwd(buf, (size_t)size);
    if (result == NULL) {
        return sys_seterr_posix();
    }

    return 0;
}

int sys_chdir(const char *path) {
    if (path == NULL) {
        sys_seterr(ERR_IO);
        return -1;
    }

    if (chdir(path) < 0) {
        return sys_seterr_posix();
    }

    return 0;
}
