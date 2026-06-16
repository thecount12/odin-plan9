#ifndef PROCESS_H
#define PROCESS_H

#include "sysdeps.h"

typedef int pid_t;

pid_t sys_getpid(void);
pid_t sys_getppid(void);
uint32 sys_getuid(void);
uint32 sys_getgid(void);
pid_t sys_fork(void);
int sys_exec(char *path, char *argv[]);
void sys_exit(int status);
pid_t sys_wait(int *status);
int sys_getpriority(int which);
int sys_setpriority(int which, int value);

#endif
