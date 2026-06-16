#include "include/sys_time.h"
#include <sys/time.h>
#include <unistd.h>

time_t sys_time(void) {
    return (time_t)time(NULL);
}

void sys_sleep(int ms) {
    struct timeval tv;

    if (ms <= 0) {
        return;
    }

    tv.tv_sec = ms / 1000;
    tv.tv_usec = (ms % 1000) * 1000;

    select(0, NULL, NULL, NULL, &tv);
}

ulonglong sys_nanotime(void) {
    struct timeval tv;
    ulonglong sec;
    ulonglong usec;

    gettimeofday(&tv, NULL);
    sec = (ulonglong)tv.tv_sec;
    usec = (ulonglong)tv.tv_usec;
    return (sec * (ulonglong)1000000 + usec) * (ulonglong)1000;
}
