#include <stdio.h>
#include "include/filesys.h"
#include "include/mem.h"
#include "include/sys_time.h"

int main(void) {
    int failed = 0;
    const char *test_path = "/tmp/odin_posix_test.txt";
    const char *test_str;
    fd_t fd;
    long written;
    char buf[64];
    long nread;
    Stat st;

    fd = sys_open(test_path, O_WRONLY | O_CREATE | O_TRUNC);
    if (fd < 0) {
        printf("FAIL: Could not create test file\n");
        failed = 1;
    } else {
        test_str = "Hello from Odin POSIX backend";
        written = sys_write(fd, test_str, (long)sys_strlen(test_str));

        if (written <= 0 || written != (long)sys_strlen(test_str)) {
            printf("FAIL: Write incorrect length\n");
            failed = 1;
        } else {
            printf("PASS: File write operations\n");
        }

        sys_close(fd);
    }

    fd = sys_open(test_path, O_RDONLY);
    if (fd < 0) {
        printf("FAIL: Could not open file for reading\n");
        failed = 1;
    } else {
        nread = sys_read(fd, buf, sizeof(buf) - 1);

        if (nread <= 0) {
            printf("FAIL: Read returned %ld\n", nread);
            failed = 1;
        } else {
            buf[nread] = '\0';
            printf("PASS: File read operations (read '%s')\n", buf);
        }

        sys_close(fd);
    }

    if (sys_stat(test_path, &st) != 0) {
        printf("FAIL: stat failed\n");
        failed = 1;
    } else {
        printf("PASS: stat (size=%llu)\n", st.length);
    }

    sys_unlink(test_path);

    if (!failed) {
        printf("\nAll file system tests passed!\n");
    }

    return failed ? 1 : 0;
}
