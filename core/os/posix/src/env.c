#include "include/env.h"
#include <stdlib.h>
#include <errno.h>

const char* sys_getenv(const char *key) {
    if (key == NULL) {
        sys_seterr(ERR_IO);
        return NULL;
    }

    return getenv(key);
}

int sys_setenv(const char *key, const char *value) {
    if (key == NULL || value == NULL) {
        sys_seterr(ERR_IO);
        return -1;
    }

    if (setenv(key, value, 1) < 0) {
        return sys_seterr_posix();
    }

    return 0;
}

int sys_unsetenv(const char *key) {
    if (key == NULL) {
        sys_seterr(ERR_IO);
        return -1;
    }

    if (unsetenv(key) < 0) {
        return sys_seterr_posix();
    }

    return 0;
}
