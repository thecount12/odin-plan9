#include "common.h"
#include "sysdeps.h"
#include "mmap.h"

/* segprotect(2) page bits — not always in libc.h on all $objtype */
#ifndef PG_R
#define PG_R 4
#define PG_W 2
#define PG_X 1
#endif

static ulong
odin_to_pg(int prot)
{
	ulong pg;

	pg = 0;
	if(prot & ODIN_PROT_READ)
		pg |= PG_R;
	if(prot & ODIN_PROT_WRITE)
		pg |= PG_W;
	if(prot & ODIN_PROT_EXEC)
		pg |= PG_X;
	return pg;
}

void *
sys_mmap(void *addr, ulong length, int prot, int flags, fd_t fd, ulong offset)
{
	char *p;
	ulong pg;

	USED(fd);
	USED(offset);

	if(length == 0) {
		sys_seterr(ERR_IO);
		return nil;
	}
	if((flags & ODIN_MAP_ANONYMOUS) == 0) {
		sys_seterr(ERR_IO);
		return nil;
	}

	p = segattach(nil, "memory", addr, length);
	if(p == nil)
		return nil;

	pg = odin_to_pg(prot);
	if(segprotect(p, length, pg) < 0) {
		segdetach(p);
		sys_seterr_plan9();
		return nil;
	}
	return p;
}

int
sys_munmap(void *addr, ulong length)
{
	USED(length);
	if(addr == nil) {
		sys_seterr(ERR_IO);
		return -1;
	}
	if(segdetach(addr) < 0)
		return sys_seterr_plan9();
	return 0;
}

int
sys_mprotect(void *addr, ulong length, int prot)
{
	if(addr == nil || length == 0) {
		sys_seterr(ERR_IO);
		return -1;
	}
	if(segprotect(addr, length, odin_to_pg(prot)) < 0)
		return sys_seterr_plan9();
	return 0;
}
