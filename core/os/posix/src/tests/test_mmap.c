#include <stdio.h>
#include "include/mmap.h"
#include "include/mem.h"

#define TEST_PAGE_SIZE 4096

int
main(void)
{
    void *mem;
    int *word;
    int failed;

    failed = 0;

    mem = sys_mmap(NULL, TEST_PAGE_SIZE,
        ODIN_PROT_READ | ODIN_PROT_WRITE,
        ODIN_MAP_PRIVATE | ODIN_MAP_ANONYMOUS,
        -1, 0);
    if (mem == NULL) {
        printf("FAIL: mmap\n");
        return 1;
    }

    word = (int *)mem;
    *word = 0x12345678;
    if (*word != 0x12345678) {
        printf("FAIL: mmap read/write\n");
        failed = 1;
    } else {
        printf("PASS: anonymous mmap read/write\n");
    }

    if (sys_mprotect(mem, TEST_PAGE_SIZE, ODIN_PROT_READ) != 0) {
        printf("FAIL: mprotect\n");
        failed = 1;
    } else {
        printf("PASS: mprotect (read-only)\n");
    }

    if (sys_munmap(mem, TEST_PAGE_SIZE) != 0) {
        printf("FAIL: munmap\n");
        failed = 1;
    } else {
        printf("PASS: munmap\n");
    }

    if (!failed) {
        printf("\nAll mmap tests passed!\n");
    }

    return failed ? 1 : 0;
}
