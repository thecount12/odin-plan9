#include "common.h"
#include "sysdeps.h"
#include "path.h"

int
sys_getcwd(char *buf, long size)
{
	if(buf == nil || size <= 0) {
		sys_seterr(ERR_IO);
		return -1;
	}
	if(getwd(buf, (int)size) == nil)
		return sys_seterr_plan9();
	return 0;
}

int
sys_chdir(char *path)
{
	if(path == nil) {
		sys_seterr(ERR_IO);
		return -1;
	}
	if(chdir(path) < 0)
		return sys_seterr_plan9();
	return 0;
}
