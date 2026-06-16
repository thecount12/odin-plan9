#ifndef SYSDEPS_H
#define SYSDEPS_H

typedef unsigned long ulong;
typedef unsigned int uint32;
typedef unsigned long ulonglong;

enum {
    ERR_OK = 0,
    ERR_IO = 5,
    ERR_NOENT = 2,
    ERR_PERM = 1
};

/* Odin-compatible file type bits (POSIX-ish; not Plan 9 DMDIR). */
enum {
    ODIN_S_IFREG = 0100000,
    ODIN_S_IFDIR = 0040000
};

typedef int fd_t;
#define MAX_FD 64

typedef struct Buf {
    unsigned char *data;
    long len;
    long cap;
} Buf;

extern int errno_;

int sys_err(void);
void sys_seterr(int err);
int sys_seterr_plan9(void);

#endif
