#include <u.h>
#include <libc.h>
#include "include/filesys.h"
#include "include/mem.h"
#include "include/sysdeps.h"

static int
odin_to_plan9_mode(int omode)
{
	int mode;

	mode = 0;
	if((omode & 3) == ODIN_O_RDONLY)
		mode |= OREAD;
	else if((omode & 3) == ODIN_O_WRONLY)
		mode |= OWRITE;
	else if((omode & 3) == ODIN_O_RDWR)
		mode |= ORDWR;
	if(omode & ODIN_O_TRUNC)
		mode |= OTRUNC;
	if(omode & ODIN_O_EXCL)
		mode |= OEXCL;
	return mode;
}

int
sys_open(char *path, int omode)
{
	int fd;
	int mode;

	if(path == nil) {
		sys_seterr(ERR_IO);
		return -1;
	}

	if(omode & ODIN_O_CREATE) {
		mode = odin_to_plan9_mode(omode & ~ODIN_O_CREATE);
		fd = create(path, mode, 0666);
	} else {
		mode = odin_to_plan9_mode(omode);
		fd = open(path, mode);
	}
	if(fd < 0)
		return sys_seterr_plan9();
	return fd;
}

int
sys_close(fd_t fd)
{
	if(fd < 0 || fd >= MAX_FD) {
		sys_seterr(ERR_IO);
		return -1;
	}
	close(fd);
	return 0;
}

long
sys_read(fd_t fd, void *buf, long count)
{
	long n;

	if(fd < 0 || fd >= MAX_FD || buf == nil || count < 0) {
		sys_seterr(ERR_IO);
		return -1;
	}
	n = read(fd, buf, count);
	if(n < 0)
		return sys_seterr_plan9();
	return n;
}

long
sys_write(fd_t fd, void *buf, long count)
{
	long n;

	if(fd < 0 || fd >= MAX_FD || buf == nil || count < 0) {
		sys_seterr(ERR_IO);
		return -1;
	}
	n = write(fd, buf, count);
	if(n < 0)
		return sys_seterr_plan9();
	return n;
}

ulong
sys_seek(fd_t fd, ulong offset, int whence)
{
	vlong result;

	if(fd < 0 || fd >= MAX_FD) {
		sys_seterr(ERR_IO);
		return (ulong)-1;
	}
	result = seek(fd, offset, whence);
	if(result < 0)
		return (ulong)sys_seterr_plan9();
	return (ulong)result;
}

int
sys_mkdir(char *path, uint32 mode)
{
	int fd;

	if(path == nil) {
		sys_seterr(ERR_IO);
		return -1;
	}
	fd = create(path, OREAD, DMDIR | (mode & 0777));
	if(fd < 0)
		return sys_seterr_plan9();
	close(fd);
	return 0;
}

int
sys_rmdir(char *path)
{
	if(path == nil) {
		sys_seterr(ERR_IO);
		return -1;
	}
	if(remove(path) < 0)
		return sys_seterr_plan9();
	return 0;
}

int
sys_rename(char *oldpath, char *newpath)
{
	if(oldpath == nil || newpath == nil) {
		sys_seterr(ERR_IO);
		return -1;
	}
	if(rename(oldpath, newpath) < 0)
		return sys_seterr_plan9();
	return 0;
}

int
sys_unlink(char *path)
{
	if(path == nil) {
		sys_seterr(ERR_IO);
		return -1;
	}
	if(remove(path) < 0)
		return sys_seterr_plan9();
	return 0;
}

int
sys_stat(char *path, Stat *st)
{
	Dir *d;

	if(path == nil || st == nil) {
		sys_seterr(ERR_IO);
		return -1;
	}
	d = dirstat(path);
	if(d == nil)
		return sys_seterr_plan9();

	memset(st, 0, sizeof(*st));
	st->type = d->mode & S_IFMT;
	st->dev = d->qid.type;
	st->ino = d->qid.path;
	st->mode = d->mode;
	st->nlink = 1;
	st->length = d->length;
	st->atime = d->atime;
	st->mtime = d->mtime;
	st->ctime = d->mtime;
	if(d->name != nil)
		strncpy(st->name, d->name, sizeof(st->name) - 1);
	free(d);
	return 0;
}
