#ifndef FILESYS_H
#define FILESYS_H


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
    uint32 type;
    uint32 dev;
    ulonglong ino;
    uint32 mode;
    uint32 nlink;
    uint32 uid;
    uint32 gid;
    uint32 rdev;
    ulonglong length;
    long atime;
    long mtime;
    long ctime;
    char name[256];
} Stat;

int sys_open(char *path, int omode);
long sys_read(fd_t fd, void *buf, long count);
long sys_write(fd_t fd, void *buf, long count);
int sys_close(fd_t fd);
ulong sys_seek(fd_t fd, ulong offset, int whence);
int sys_unlink(char *name);
int sys_stat(char *path, Stat *st);
int sys_mkdir(char *path, uint32 mode);
int sys_rmdir(char *path);
int sys_rename(char *oldpath, char *newpath);

#endif
