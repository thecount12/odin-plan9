#ifndef SYSDEPS_H
#define SYSDEPS_H

#include <stddef.h>
#include <stdint.h>

typedef unsigned long ulong;
typedef uint32_t uint32;
typedef uint64_t ulonglong;   /* C89-safe via <stdint.h> */
typedef int64_t longlong;

enum {
    ERR_OK = 0,
    ERR_IO = 5,
    ERR_NOENT = 2,
    ERR_PERM = 1
};

typedef int fd_t;
#define MAX_FD 64

/* Use unsigned char instead of uchar, uint32/uint64 -> explicit stdint */
typedef struct Buf {
    unsigned char *data;
    long len;
    long cap;
} Buf;

extern int errno_;

int sys_err(void);
void sys_seterr(int err);
int sys_seterr_posix(void);

#endif
