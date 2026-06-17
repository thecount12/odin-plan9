#include "common.h"
#include "sysdeps.h"
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
	uvlong ns;

	ns = (uvlong)time(0) * 1000000000;
	return sys_ull_from_ptr(&ns);
}
