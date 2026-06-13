#include <stdio.h>
#include "include/path.h"
#include "include/filesys.h"

int main(void) {
    int failed = 0;
    char cwd[SYS_PATH_MAX];
    char inside[SYS_PATH_MAX];
    const char *subdir = "/tmp/odin_posix_path_test";

    if (sys_getcwd(cwd, (long)sizeof(cwd)) != 0) {
        printf("FAIL: getcwd\n");
        failed = 1;
    } else {
        printf("PASS: getcwd = '%s'\n", cwd);
    }

    sys_mkdir(subdir, 0755);

    if (sys_chdir(subdir) != 0) {
        printf("FAIL: chdir into test dir\n");
        failed = 1;
    } else {
        if (sys_getcwd(inside, (long)sizeof(inside)) != 0) {
            printf("FAIL: getcwd after chdir\n");
            failed = 1;
        } else {
            printf("PASS: chdir (now in '%s')\n", inside);
        }
        sys_chdir(cwd);
    }

    sys_rmdir(subdir);

    return failed ? 1 : 0;
}
