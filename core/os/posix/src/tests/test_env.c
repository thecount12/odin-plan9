#include <stdio.h>
#include "include/env.h"
#include "include/mem.h"

int main(void) {
    int failed = 0;
    const char *key = "ODIN_POSIX_TEST_VAR";
    const char *value = "hello_posix";
    const char *got;

    if (sys_setenv(key, value) != 0) {
        printf("FAIL: setenv\n");
        failed = 1;
    } else {
        got = sys_getenv(key);
        if (got == NULL || sys_memcmp(got, value, (size_t)sys_strlen(value) + 1) != 0) {
            printf("FAIL: getenv after setenv\n");
            failed = 1;
        } else {
            printf("PASS: setenv/getenv ('%s')\n", got);
        }
    }

    if (sys_unsetenv(key) != 0) {
        printf("FAIL: unsetenv\n");
        failed = 1;
    } else {
        got = sys_getenv(key);
        if (got != NULL) {
            printf("FAIL: getenv after unsetenv\n");
            failed = 1;
        } else {
            printf("PASS: unsetenv\n");
        }
    }

    return failed ? 1 : 0;
}
