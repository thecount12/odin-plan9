#include <stdio.h>
#include "include/time.h"

int main(void) {
    int failed = 0;
    time_t t1;
    ulonglong n1;

    t1 = sys_time();
    if (t1 <= 0) {
        printf("FAIL: time returned %ld\n", t1);
        failed = 1;
    } else {
        printf("PASS: time = %ld\n", t1);
    }

    printf("Testing sleep(100ms)... ");
    sys_sleep(100);
    printf("done.\n");

    n1 = sys_nanotime();
    if (n1 <= 0) {
        printf("FAIL: nanotime returned %llu\n", n1);
        failed = 1;
    } else {
        printf("PASS: nanotime = %llu\n", n1);
    }

    return failed ? 1 : 0;
}
