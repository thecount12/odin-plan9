#include <stdio.h>
#include "include/mmap.h"
#include "include/filesys.h"
#include "include/mem.h"

#define TEST_SIZE 4096

int main(void) {
    int failed = 0;
    void *addr;
    int *mapped;
    const char *path = "/tmp/odin_posix_mmap_test.bin";
    fd_t fd;
    const char *text;
    long written;
    void *file_map;
    char *bytes;

    addr = sys_mmap(NULL, TEST_SIZE,
                    PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANON,
                    -1, 0);
    if (addr == SYS_MAP_FAILED) {
        printf("FAIL: anonymous mmap\n");
        failed = 1;
    } else {
        mapped = (int *)addr;
        *mapped = 42;

        if (sys_mprotect(addr, TEST_SIZE, PROT_READ) != 0) {
            printf("FAIL: mprotect to read-only\n");
            failed = 1;
        } else if (*mapped != 42) {
            printf("FAIL: read back after mprotect\n");
            failed = 1;
        } else {
            printf("PASS: anonymous mmap and mprotect\n");
        }

        if (sys_munmap(addr, TEST_SIZE) != 0) {
            printf("FAIL: munmap anonymous region\n");
            failed = 1;
        } else {
            printf("PASS: munmap anonymous region\n");
        }
    }

    text = "mmap file test";
    fd = sys_open(path, O_WRONLY | O_CREATE | O_TRUNC);
    if (fd < 0) {
        printf("FAIL: could not create mmap test file\n");
        failed = 1;
    } else {
        written = sys_write(fd, text, (long)sys_strlen(text));
        sys_close(fd);

        if (written != (long)sys_strlen(text)) {
            printf("FAIL: could not write mmap test file\n");
            failed = 1;
        } else {
            fd = sys_open(path, O_RDONLY);
            if (fd < 0) {
                printf("FAIL: could not open mmap test file\n");
                failed = 1;
            } else {
                file_map = sys_mmap(NULL, TEST_SIZE,
                                    PROT_READ,
                                    MAP_PRIVATE,
                                    (int)fd, 0);
                if (file_map == SYS_MAP_FAILED) {
                    printf("FAIL: file-backed mmap\n");
                    failed = 1;
                } else {
                    bytes = (char *)file_map;
                    if (sys_memcmp(bytes, text, (size_t)sys_strlen(text)) != 0) {
                        printf("FAIL: file-backed mmap contents\n");
                        failed = 1;
                    } else {
                        printf("PASS: file-backed mmap\n");
                    }
                    sys_munmap(file_map, TEST_SIZE);
                }
                sys_close(fd);
            }
        }
        sys_unlink(path);
    }

    if (!failed) {
        printf("\nAll mmap tests passed!\n");
    }

    return failed ? 1 : 0;
}
