#include "include/process.h"
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

pid_t sys_getpid(void) {
    return (pid_t)getpid();
}

pid_t sys_getppid(void) {
    return (pid_t)getppid();
}

uint32 sys_getuid(void) {
    return (uint32)getuid();
}

uint32 sys_getgid(void) {
    return (uint32)getgid();
}

pid_t sys_fork(void) {
    return (pid_t)fork();
}

int sys_exec(const char *path, char *const argv[]) {
    if (path == NULL) {
        sys_seterr(ERR_IO);
        return -1;
    }
    return execv(path, argv);
}

void sys_exit(int status) {
    _exit(status);
}

pid_t sys_wait(int *status) {
    pid_t pid;

    pid = wait(status);
    if (pid < 0) {
        sys_seterr_posix();
    }
    return pid;
}

int sys_getpriority(int which) {
    return getpriority(which, 0);
}

int sys_setpriority(int which, int value) {
    return setpriority(which, 0, value);
}
