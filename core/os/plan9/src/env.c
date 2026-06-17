#include "common.h"
#include "sysdeps.h"
#include "env.h"

char *
sys_getenv(char *key)
{
	if(key == nil) {
		sys_seterr(ERR_IO);
		return nil;
	}
	return getenv(key);
}

int
sys_setenv(char *key, char *value)
{
	if(key == nil || value == nil) {
		sys_seterr(ERR_IO);
		return -1;
	}
	if(putenv(key, value) < 0)
		return sys_seterr_plan9();
	return 0;
}

int
sys_unsetenv(char *key)
{
	char path[256];

	if(key == nil) {
		sys_seterr(ERR_IO);
		return -1;
	}
	if(getenv(key) == nil)
		return 0;
	snprint(path, sizeof path, "/env/%s", key);
	if(remove(path) < 0)
		return sys_seterr_plan9();
	return 0;
}
