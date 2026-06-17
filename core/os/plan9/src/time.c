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
	if(ms > 0)
		sleep(ms);
}

ulonglong
sys_nanotime(void)
{
	uvlong ns;

	ns = (uvlong)time(0) * 1000000000;
	return sys_ull_from_ptr(&ns);
}
