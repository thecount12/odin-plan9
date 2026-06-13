#include <stdio.h>
#include "include/sysdeps.h"
#include "include/filesys.h"
#include "include/process.h"
#include "include/mem.h"
#include "include/time.h"

int main(int argc, char *argv[]) {
    int failed = 0;

    printf("Testing POSIX C89 backend...\n\n");

    /* Test memory management */
    {
        void *p = sys_malloc(64);
        if (p == NULL) {
            printf("FAIL: malloc returned NULL\n");
            failed = 1;
        } else {
            sys_memset(p, 0, 64);
            printf("PASS: Memory allocation\n");
            sys_free(p);
        }
    }

    /* Test file operations */
    {
        fd_t fd = sys_open("/tmp/test_posix_c89.txt", O_WRONLY | O_CREATE | O_TRUNC);
        if (fd < 0) {
            printf("FAIL: Could not create test file\n");
            failed = 1;
        } else {
            const char *test_str = "POSIX C89 Test";
            long written = sys_write(fd, test_str, (long)sys_strlen(test_str));
            if (written <= 0) {
                printf("FAIL: Could not write to file\n");
                failed = 1;
            } else {
                printf("PASS: File I/O operations\n");
            }
            sys_close(fd);
        }
    }

    /* Test time functions */
    {
        time_t t = sys_time();
        if (t == 0) {
            printf("WARN: Time returned 0\n");
        } else {
            printf("PASS: Time function (%ld)\n", t);
        }
    }

    if (!failed) {
        printf("\nAll basic tests passed!\n");
    } else {
        printf("\nSome tests failed.\n");
    }

    return failed ? 1 : 0;
}
