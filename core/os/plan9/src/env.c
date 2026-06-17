#include "common.h"
#include "sysdeps.h"
#include "env.h"
#include "mem.h"

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
	char *buf;
	int len;

	if(key == nil || value == nil) {
		sys_seterr(ERR_IO);
		return -1;
	}

	len = strlen(key) + strlen(value) + 2;
	buf = sys_malloc(len);
	if(buf == nil) {
		sys_seterr(ERR_IO);
		return -1;
	}
	snprint(buf, len, "%s=%s", key, value);
	if(putenv(buf) < 0) {
		sys_free(buf);
		return sys_seterr_plan9();
	}
	/* putenv keeps buf; do not free */
	return 0;
}

int
sys_unsetenv(char *key)
{
	if(key == nil) {
		sys_seterr(ERR_IO);
		return -1;
	}
	if(unsetenv(key) < 0)
		return sys_seterr_plan9();
	return 0;
}
