#ifndef FILESYS_H
#define FILESYS_H

#include "sysdeps.h"

/* Odin/Plan 9 open flags (distinct from POSIX <fcntl.h> values) */
enum {
    ODIN_O_RDONLY = 0x000,
    ODIN_O_WRONLY = 0x001,
    ODIN_O_RDWR   = 0x002,
    ODIN_O_CREATE = 0x040,
    ODIN_O_TRUNC  = 0x200,
    ODIN_O_EXCL   = 0x080
};

#define O_RDONLY ODIN_O_RDONLY
#define O_WRONLY ODIN_O_WRONLY
#define O_RDWR   ODIN_O_RDWR
#define O_CREATE ODIN_O_CREATE
#define O_TRUNC  ODIN_O_TRUNC
#define O_EXCL   ODIN_O_EXCL

typedef struct Stat {
    uint32_t type;      /* Note: must match Plan9's layout for compatibility */
    uint32_t dev;
    uint64_t ino;
    uint32_t mode;
    uint32_t nlink;
    uint32_t uid;
    uint32_t gid;
    uint32_t rdev;
    uint64_t length;
    long atime;
    long mtime;
    long ctime;
    char name[256];
} Stat;

int sys_open(const char *path, int omode);
long sys_read(fd_t fd, void *buf, long count);
long sys_write(fd_t fd, const void *buf, long count);
int sys_close(fd_t fd);
ulong sys_seek(fd_t fd, ulong offset, int whence);
int sys_unlink(const char *name);
int sys_stat(const char *path, Stat *st);
int sys_mkdir(const char *path, uint32_t mode);
int sys_rmdir(const char *path);
int sys_rename(const char *oldpath, const char *newpath);

#endif