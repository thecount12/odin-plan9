#include "common.h"
#include "sysdeps.h"
#include "dir.h"
#include "mem.h"

struct SysDir {
	int fd;
	Dir *dbuf;
	long nent;
	long index;
};

static void
free_dirbuf(SysDir *dir)
{
	long i;

	if(dir == nil || dir->dbuf == nil)
		return;
	for(i = 0; i < dir->nent; i++) {
		if(dir->dbuf[i].name != nil)
			free(dir->dbuf[i].name);
	}
	free(dir->dbuf);
	dir->dbuf = nil;
	dir->nent = 0;
	dir->index = 0;
}

SysDir *
sys_opendir(char *path)
{
	SysDir *dir;
	long n;

	if(path == nil) {
		sys_seterr(ERR_IO);
		return nil;
	}

	dir = sys_malloc(sizeof(SysDir));
	if(dir == nil) {
		sys_seterr(ERR_IO);
		return nil;
	}

	dir->fd = open(path, OREAD);
	if(dir->fd < 0) {
		sys_free(dir);
		sys_seterr_plan9();
		return nil;
	}

	dir->dbuf = nil;
	dir->nent = 0;
	dir->index = 0;

	n = dirread(dir->fd, &dir->dbuf);
	close(dir->fd);
	dir->fd = -1;
	if(n < 0) {
		free_dirbuf(dir);
		sys_free(dir);
		sys_seterr_plan9();
		return nil;
	}
	dir->nent = n;
	return dir;
}

int
sys_readdir(SysDir *dir, DirEnt *ent)
{
	Dir *d;

	if(dir == nil || ent == nil) {
		sys_seterr(ERR_IO);
		return -1;
	}
	if(dir->index >= dir->nent)
		return 0;

	d = &dir->dbuf[dir->index++];
	sys_memset(ent, 0, sizeof(DirEnt));
	if(d->name != nil) {
		strncpy(ent->name, d->name, sizeof(ent->name) - 1);
		ent->name[sizeof(ent->name) - 1] = '\0';
	}
	ent->is_dir = (d->mode & DMDIR) ? 1 : 0;
	return 1;
}

int
sys_closedir(SysDir *dir)
{
	if(dir == nil) {
		sys_seterr(ERR_IO);
		return -1;
	}
	if(dir->fd >= 0)
		close(dir->fd);
	free_dirbuf(dir);
	sys_free(dir);
	return 0;
}
