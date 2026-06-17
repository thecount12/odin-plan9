/*
 * Example generated program for Plan 9 integration testing.
 */
#include "common.h"
#include "sysdeps.h"
#include "mem.h"
#include "filesys.h"

int
odin_main(int argc, char **argv)
{
	char *msg;

	USED(argc);
	USED(argv);
	msg = "hello from odin plan9\n";
	sys_write(1, msg, sys_strlen(msg));
	return 0;
}
