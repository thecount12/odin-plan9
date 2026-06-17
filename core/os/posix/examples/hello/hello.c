/*
 * Example generated program for POSIX C89 integration testing.
 */
#include "include/filesys.h"
#include "include/mem.h"

int
odin_main(int argc, char **argv)
{
	const char *msg;

	(void)argc;
	(void)argv;
	msg = "hello from odin posix-c89\n";
	sys_write(1, msg, (long)sys_strlen(msg));
	return 0;
}
