#include <stdio.h>
#include <sys/wait.h>
#include "include/process.h"
#include "include/mem.h"

int main(void) {
    int failed = 0;
    pid_t pid;
    pid_t ppid;
    pid_t child;
    int status;
    pid_t wpid;

    pid = sys_getpid();
    if (pid <= 0) {
        printf("FAIL: getpid returned %d\n", pid);
        failed = 1;
    } else {
        printf("PASS: getpid = %d\n", pid);
    }

    ppid = sys_getppid();
    if (ppid < 0) {
        printf("FAIL: getppid returned %d\n", ppid);
        failed = 1;
    } else {
        printf("PASS: getppid = %d\n", ppid);
    }

    child = sys_fork();
    if (child < 0) {
        printf("FAIL: fork failed\n");
        failed = 1;
    } else if (child == 0) {
        sys_exit(42);
    } else {
        wpid = sys_wait(&status);
        if (wpid != child) {
            printf("FAIL: wait returned wrong pid\n");
            failed = 1;
        } else if (!WIFEXITED(status) || WEXITSTATUS(status) != 42) {
            printf("FAIL: child exit status incorrect\n");
            failed = 1;
        } else {
            printf("PASS: fork/wait cycle\n");
        }
    }

    return failed ? 1 : 0;
}
