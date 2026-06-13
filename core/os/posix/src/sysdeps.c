#include "include/sysdeps.h"
#include <errno.h>

int errno_;

int sys_err(void) {
    return errno_;
}

void sys_seterr(int err) {
    errno_ = err;
}

int sys_seterr_posix(void) {
    errno_ = errno;
    return -1;
}
