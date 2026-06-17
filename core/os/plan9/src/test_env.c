#include "common.h"
#include "sysdeps.h"
#include "env.h"
#include "mem.h"

void
main(void)
{
	int failed;
	char *key;
	char *value;
	char *got;

	failed = 0;
	key = "ODIN_PLAN9_TEST_VAR";
	value = "hello_plan9";

	if(sys_setenv(key, value) != 0) {
		print("FAIL: setenv\n");
		failed = 1;
	} else {
		got = sys_getenv(key);
		if(got == nil || sys_memcmp(got, value, sys_strlen(value) + 1) != 0) {
			print("FAIL: getenv after setenv\n");
			failed = 1;
		} else {
			print("PASS: setenv/getenv ('%s')\n", got);
		}
	}

	if(sys_unsetenv(key) != 0) {
		print("FAIL: unsetenv\n");
		failed = 1;
	} else {
		got = sys_getenv(key);
		if(got != nil) {
			print("FAIL: getenv after unsetenv\n");
			failed = 1;
		} else {
			print("PASS: unsetenv\n");
		}
	}

	if(!failed)
		print("\nAll environment tests passed!\n");

	exits(failed ? "fail" : nil);
}
