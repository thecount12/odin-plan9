#include <stdio.h>
#include "include/sysdeps.h"
#include "include/thread.h"

static sys_mutex_t mutex;
static int shared_counter = 0;

void* increment(void *arg) {
    int i;

    (void)arg;
    for (i = 0; i < 1000; i++) {
        sys_mutex_lock(&mutex);
        shared_counter++;
        sys_mutex_unlock(&mutex);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    int failed = 0;
    sys_thread_t thread1;
    sys_thread_t thread2;
    void *res1;
    void *res2;

    (void)argc;
    (void)argv;

    printf("Testing POSIX C89 backend... (Threading)\n\n");

    if (sys_mutex_init(&mutex, NULL) != 0) {
        printf("FAIL: Mutex initialization failed\n");
        failed = 1;
    } else {
        if (sys_thread_create(&thread1, NULL, increment, NULL) != 0) {
            printf("FAIL: Thread creation (1) failed\n");
            failed = 1;
        }

        if (sys_thread_create(&thread2, NULL, increment, NULL) != 0) {
            printf("FAIL: Thread creation (2) failed\n");
            failed = 1;
        } else {
            sys_thread_join(thread1, &res1);
            sys_thread_join(thread2, &res2);

            if (res1 != NULL || res2 != NULL) {
                printf("FAIL: Thread join failed\n");
                failed = 1;
            } else if (shared_counter != 2000) {
                printf("FAIL: Incorrect counter value (%d)\n", shared_counter);
                failed = 1;
            } else {
                printf("PASS: Threading and synchronization\n");
            }
        }

        sys_mutex_destroy(&mutex);
    }

    if (!failed) {
        printf("\nAll threading tests passed!\n");
    } else {
        printf("\nSome threading tests failed.\n");
    }

    return failed ? 1 : 0;
}
