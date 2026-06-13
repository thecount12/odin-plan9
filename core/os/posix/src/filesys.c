#include "include/filesys.h"
#include "include/mem.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

/* Map Odin flags → POSIX equivalents at runtime */
static int odin_to_posix_flags(int omode) {
    int posix_mode = 0;

    if (omode & ODIN_O_RDONLY) posix_mode |= O_RDONLY;
    if (omode & ODIN_O_WRONLY) posix_mode |= O_WRONLY;
    if (omode & ODIN_O_RDWR)   posix_mode |= O_RDWR;
    if (omode & ODIN_O_CREATE) posix_mode |= O_CREAT;
    if (omode & ODIN_O_TRUNC)  posix_mode |= O_TRUNC;
    if (omode & ODIN_O_EXCL)   posix_mode |= O_EXCL;

    return posix_mode;
}


int sys_open(const char *path, int omode) {
    int mode;

    if (path == NULL) {
        sys_seterr(ERR_IO);
        return -1;
    }

    mode = odin_to_posix_flags(omode);
    return open(path, mode, 0666);
}

int sys_close(fd_t fd) {
    if (fd < 0 || fd >= MAX_FD) {
        sys_seterr(ERR_IO);
        return -1;
    }
    return close((int)fd);
}

long sys_read(fd_t fd, void *buf, long count) {
    ssize_t n;

    if (fd < 0 || fd >= MAX_FD || buf == NULL || count < 0) {
        sys_seterr(ERR_IO);
        return -1;
    }

    n = read((int)fd, buf, (size_t)count);
    if (n < 0) {
        sys_seterr_posix();
        return -1;
    }
    return (long)n;
}

long sys_write(fd_t fd, const void *buf, long count) {
    ssize_t n;

    if (fd < 0 || fd >= MAX_FD || buf == NULL || count < 0) {
        sys_seterr(ERR_IO);
        return -1;
    }

    n = write((int)fd, buf, (size_t)count);
    if (n < 0) {
        sys_seterr_posix();
        return -1;
    }
    return (long)n;
}

ulong sys_seek(fd_t fd, ulong offset, int whence) {
    off_t result;

    if (fd < 0 || fd >= MAX_FD) {
        sys_seterr(ERR_IO);
        return (ulong)-1;
    }

    result = lseek((int)fd, (off_t)offset, whence);
    if (result == -1) {
        sys_seterr_posix();
        return (ulong)-1;
    }
    return (ulong)result;
}

int sys_mkdir(const char *path, uint32_t mode) {
    if (path == NULL) {
        sys_seterr(ERR_IO);
        return -1;
    }
    return mkdir(path, (mode_t)mode);
}

int sys_rmdir(const char *path) {
    if (path == NULL) {
        sys_seterr(ERR_IO);
        return -1;
    }
    return rmdir(path);
}

int sys_rename(const char *oldpath, const char *newpath) {
    if (oldpath == NULL || newpath == NULL) {
        sys_seterr(ERR_IO);
        return -1;
    }
    return rename(oldpath, newpath);
}

int sys_unlink(const char *path) {
    if (path == NULL) {
        sys_seterr(ERR_IO);
        return -1;
    }
    return unlink(path);
}

int sys_stat(const char *path, Stat *st) {
    struct stat st_;

    if (path == NULL || st == NULL) {
        sys_seterr(ERR_IO);
        return -1;
    }

    if (stat(path, &st_) < 0) {
        sys_seterr_posix();
        return -1;
    }

    st->type = st_.st_mode & S_IFMT;
    st->dev = st_.st_dev;
    st->ino = st_.st_ino;
    st->mode = st_.st_mode;
    st->nlink = st_.st_nlink;
    st->uid = st_.st_uid;
    st->gid = st_.st_gid;
    st->rdev = st_.st_rdev;
    st->length = (ulonglong)st_.st_size;
    st->atime = st_.st_atime;
    st->mtime = st_.st_mtime;
    st->ctime = st_.st_ctime;

    return 0;
}
