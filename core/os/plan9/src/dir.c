#include "common.h"
#include "sysdeps.h"
#include "dir.h"
#include "mem.h"

struct SysDir {
	int fd;
};

SysDir *
sys_opendir(char *path)
{
	SysDir *dir;

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
	return dir;
}

int
sys_readdir(SysDir *dir, DirEnt *ent)
{
	Dir d;
	long n;

	if(dir == nil || ent == nil) {
		sys_seterr(ERR_IO);
		return -1;
	}

	n = dirread(dir->fd, &d, 1);
	if(n < 0)
		return sys_seterr_plan9();
	if(n == 0)
		return 0;

	sys_memset(ent, 0, sizeof(DirEnt));
	if(d.name != nil) {
		strncpy(ent->name, d.name, sizeof(ent->name) - 1);
		ent->name[sizeof(ent->name) - 1] = '\0';
		free(d.name);
	}
	ent->is_dir = (d.mode & DMDIR) ? 1 : 0;
	return 1;
}

int
sys_closedir(SysDir *dir)
{
	if(dir == nil) {
		sys_seterr(ERR_IO);
		return -1;
	}
	close(dir->fd);
	sys_free(dir);
	return 0;
}
