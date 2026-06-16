#include <u.h>
#include <libc.h>
#include "include/process.h"
#include "include/sysdeps.h"

pid_t
sys_getpid(void)
{
	return getpid();
}

pid_t
sys_getppid(void)
{
	/* Plan 9 has no portable getppid; return 0 for now. */
	return 0;
}

uint32
sys_getuid(void)
{
	return 0;
}

uint32
sys_getgid(void)
{
	return 0;
}

pid_t
sys_fork(void)
{
	return rfork(RFPROC|RFMEM|RFFDG);
}

int
sys_exec(char *path, char *argv[])
{
	if(path == nil) {
		sys_seterr(ERR_IO);
		return -1;
	}
	if(exec(path, argv) < 0)
		return sys_seterr_plan9();
	return 0;
}

void
sys_exit(int status)
{
	if(status != 0)
		exits("exit");
	exits(nil);
}

pid_t
sys_wait(int *status)
{
	Waitmsg *w;
	int pid;

	w = wait();
	if(w == nil)
		return sys_seterr_plan9();
	pid = w->pid;
	if(status != nil) {
		if(w->msg[0] == '\0')
			*status = 0;
		else
			*status = 1;
	}
	free(w);
	return pid;
}

int
sys_getpriority(int which)
{
	USED(which);
	return 0;
}

int
sys_setpriority(int which, int value)
{
	USED(which);
	USED(value);
	return 0;
}
