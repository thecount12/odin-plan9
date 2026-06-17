#include <stdio.h>
#include "include/thread.h"
#include "include/mem.h"

static SysMutex *g_mutex;
static SysCond *g_cond;
static int g_ready;
static int g_count;

static void *
worker_fn(void *arg)
{
    int *out;

    out = (int *)arg;
    (void)out;

    if (sys_mutex_lock(g_mutex) != 0) {
        return NULL;
    }

    while (!g_ready) {
        if (sys_cond_wait(g_cond, g_mutex) != 0) {
            sys_mutex_unlock(g_mutex);
            return NULL;
        }
    }

    g_count += 1;
    sys_mutex_unlock(g_mutex);
    return NULL;
}

int
main(void)
{
    int failed;
    SysThread *thread;
    void *retval;

    failed = 0;
    g_count = 0;
    g_ready = 0;
    g_mutex = sys_mutex_create();
    g_cond = sys_cond_create();

    if (g_mutex == NULL || g_cond == NULL) {
        printf("FAIL: create mutex/cond\n");
        return 1;
    }

    if (sys_thread_create(worker_fn, NULL, &thread) != 0) {
        printf("FAIL: thread_create\n");
        failed = 1;
    } else {
        if (sys_mutex_lock(g_mutex) != 0) {
            printf("FAIL: mutex_lock (main)\n");
            failed = 1;
        } else {
            g_ready = 1;
            if (sys_cond_signal(g_cond) != 0) {
                printf("FAIL: cond_signal\n");
                failed = 1;
            }
            sys_mutex_unlock(g_mutex);
        }

        if (!failed && sys_thread_join(thread, &retval) != 0) {
            printf("FAIL: thread_join\n");
            failed = 1;
        } else if (!failed && g_count != 1) {
            printf("FAIL: worker count=%d expected 1\n", g_count);
            failed = 1;
        } else if (!failed) {
            printf("PASS: thread create/join with cond/mutex\n");
        }
    }

    sys_cond_destroy(g_cond);
    sys_mutex_destroy(g_mutex);

    if (!failed) {
        printf("\nAll thread tests passed!\n");
    }

    return failed ? 1 : 0;
}
