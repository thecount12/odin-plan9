#include <u.h>
#include <libc.h>
#include "include/sys_time.h"

ulong
sys_time(void)
{
	return time(0);
}

void
sys_sleep(int ms)
{
	if(ms > 0)
		nap(ms);
}

ulonglong
sys_nanotime(void)
{
	return (ulonglong)time(0) * (ulonglong)1000000000;
}
