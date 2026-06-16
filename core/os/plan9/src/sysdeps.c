#include <u.h>
#include <libc.h>
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
