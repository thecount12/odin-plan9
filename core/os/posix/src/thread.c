#include "include/thread.h"
#include <pthread.h>
#include <stdlib.h>

typedef struct {
    void *(*start_routine)(void*);
    void *arg;
} thread_args_t;

static void* thread_wrapper(void *arg) {
    thread_args_t *args;
    void *(*func)(void*);
    void *fn_arg;

    args = (thread_args_t *)arg;
    func = args->start_routine;
    fn_arg = args->arg;
    free(args);
    return func(fn_arg);
}

int sys_thread_create(sys_thread_t *thread, const sys_thread_attr_t *attr,
                      void *(*start_routine) (void *), void *arg) {
    thread_args_t *args;
    int ret;

    if (!thread || !start_routine) {
        return -1;
    }

    args = malloc(sizeof(thread_args_t));
    if (!args) {
        return -1;
    }

    args->start_routine = start_routine;
    args->arg = arg;

    ret = pthread_create((pthread_t *)thread, (const pthread_attr_t *)attr,
                         thread_wrapper, args);
    if (ret != 0) {
        free(args);
        return -1;
    }

    return 0;
}

int sys_thread_join(sys_thread_t thread, void **retval) {
    int ret;

    ret = pthread_join((pthread_t)thread, retval);
    return ret == 0 ? 0 : -1;
}

int sys_mutex_init(sys_mutex_t *mutex, const sys_mutexattr_t *attr) {
    int ret;

    if (!mutex) {
        return -1;
    }

    ret = pthread_mutex_init((pthread_mutex_t *)mutex, (const pthread_mutexattr_t *)attr);
    return ret == 0 ? 0 : -1;
}

int sys_mutex_lock(sys_mutex_t *mutex) {
    int ret;

    if (!mutex) {
        return -1;
    }

    ret = pthread_mutex_lock((pthread_mutex_t *)mutex);
    return ret == 0 ? 0 : -1;
}

int sys_mutex_unlock(sys_mutex_t *mutex) {
    int ret;

    if (!mutex) {
        return -1;
    }

    ret = pthread_mutex_unlock((pthread_mutex_t *)mutex);
    return ret == 0 ? 0 : -1;
}

int sys_mutex_destroy(sys_mutex_t *mutex) {
    int ret;

    if (!mutex) {
        return -1;
    }

    ret = pthread_mutex_destroy((pthread_mutex_t *)mutex);
    return ret == 0 ? 0 : -1;
}

int sys_cond_init(sys_cond_t *cond, const sys_condattr_t *attr) {
    int ret;

    if (!cond) {
        return -1;
    }

    ret = pthread_cond_init((pthread_cond_t *)cond, (const pthread_condattr_t *)attr);
    return ret == 0 ? 0 : -1;
}

int sys_cond_wait(sys_cond_t *cond, sys_mutex_t *mutex) {
    int ret;

    if (!cond || !mutex) {
        return -1;
    }

    ret = pthread_cond_wait((pthread_cond_t *)cond, (pthread_mutex_t *)mutex);
    return ret == 0 ? 0 : -1;
}

int sys_cond_signal(sys_cond_t *cond) {
    int ret;

    if (!cond) {
        return -1;
    }

    ret = pthread_cond_signal((pthread_cond_t *)cond);
    return ret == 0 ? 0 : -1;
}

int sys_cond_broadcast(sys_cond_t *cond) {
    int ret;

    if (!cond) {
        return -1;
    }

    ret = pthread_cond_broadcast((pthread_cond_t *)cond);
    return ret == 0 ? 0 : -1;
}

int sys_cond_destroy(sys_cond_t *cond) {
    int ret;

    if (!cond) {
        return -1;
    }

    ret = pthread_cond_destroy((pthread_cond_t *)cond);
    return ret == 0 ? 0 : -1;
}
