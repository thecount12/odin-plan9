#include "common.h"
#include "sysdeps.h"

int errno_;

int
sys_err(void)
{
	return errno_;
}

void
sys_seterr(int err)
{
	errno_ = err;
}

int
sys_seterr_plan9(void)
{
	char buf[ERRMAX];

	errstr(buf, sizeof buf);
	errno_ = ERR_IO;
	return -1;
}

ulonglong
sys_ull(ulong v)
{
	ulonglong u;

	u.lo = v;
	u.hi = 0;
	return u;
}

ulonglong
sys_ull_from_ptr(void *src)
{
	ulonglong u;

	u.lo = 0;
	u.hi = 0;
	if(src != nil)
		memmove(&u, src, sizeof(u));
	return u;
}

int
sys_ull_eq(ulonglong a, ulonglong b)
{
	return a.lo == b.lo && a.hi == b.hi;
}

int
sys_ull_eq_ulong(ulonglong u, ulong v)
{
	return u.hi == 0 && u.lo == v;
}

int
sys_ull_snprint(ulonglong u, char *buf, int n)
{
	uvlong v;

	memmove(&v, &u, sizeof(v));
	return snprint(buf, n, "%llud", v);
}
