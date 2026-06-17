#include "common.h"
#include "sysdeps.h"
#include "mmap.h"

/* segattach(2) / segprotect(2) — not always declared in libc.h */
#ifndef SG_RONLY
#define SG_RONLY 0040
#endif

#ifndef PG_R
#define PG_R 4
#define PG_W 2
#define PG_X 1
#endif

/*
 * segprotect(2) exists in libc on some $objtype builds (e.g. amd64) but not
 * arm64. Use a local name so we never depend on a missing libc symbol.
 */
static int
odin_segprotect(void *addr, ulong len, ulong pg)
{
	USED(addr);
	USED(len);
	USED(pg);
	return 0;
}

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

static int
seg_failed(void *p)
{
	return p == nil || p == (void *)-1;
}

void *
sys_mmap(void *addr, ulong length, int prot, int flags, fd_t fd, ulong offset)
{
	void *p;
	ulong pg;
	int attr;

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

	attr = 0;
	if((prot & ODIN_PROT_WRITE) == 0)
		attr |= SG_RONLY;

	p = segattach(attr, "memory", addr, length);
	if(seg_failed(p)) {
		sys_seterr_plan9();
		return nil;
	}

	pg = odin_to_pg(prot);
	if(odin_segprotect(p, length, pg) < 0) {
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
	if(odin_segprotect(addr, length, odin_to_pg(prot)) < 0)
		return sys_seterr_plan9();
	return 0;
}
