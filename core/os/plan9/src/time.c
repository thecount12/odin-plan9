#include <u.h>
#include <libc.h>
#include "sys_time.h"

ulong
sys_time(void)
{
	return time(0);
}

void
sys_sleep(int ms)
{
	ulong stop;

	if(ms <= 0)
		return;
	stop = osmillisecond() + (ulong)ms;
	while(osmillisecond() < stop)
		;
}

ulonglong
sys_nanotime(void)
{
	return (ulonglong)time(0) * (ulonglong)1000000000;
}
