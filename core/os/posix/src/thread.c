#include "include/thread.h"
#include "include/mem.h"
#include "include/sysdeps.h"
#include <pthread.h>

struct SysMutex {
    pthread_mutex_t impl;
};

struct SysCond {
    pthread_cond_t impl;
};

struct SysThread {
    pthread_t impl;
};

static int
sys_pthread_fail(int result)
{
    if (result != 0) {
        sys_seterr(ERR_IO);
        return -1;
    }
    return 0;
}

SysMutex *
sys_mutex_create(void)
{
    SysMutex *mutex;
    int result;

    mutex = (SysMutex *)sys_malloc(sizeof(SysMutex));
    if (mutex == NULL) {
        sys_seterr(ERR_IO);
        return NULL;
    }

    result = pthread_mutex_init(&mutex->impl, NULL);
    if (result != 0) {
        sys_free(mutex);
        sys_seterr(ERR_IO);
        return NULL;
    }

    return mutex;
}

int
sys_mutex_lock(SysMutex *mutex)
{
    if (mutex == NULL) {
        sys_seterr(ERR_IO);
        return -1;
    }
    return sys_pthread_fail(pthread_mutex_lock(&mutex->impl));
}

int
sys_mutex_unlock(SysMutex *mutex)
{
    if (mutex == NULL) {
        sys_seterr(ERR_IO);
        return -1;
    }
    return sys_pthread_fail(pthread_mutex_unlock(&mutex->impl));
}

void
sys_mutex_destroy(SysMutex *mutex)
{
    if (mutex == NULL) {
        return;
    }
    pthread_mutex_destroy(&mutex->impl);
    sys_free(mutex);
}

SysCond *
sys_cond_create(void)
{
    SysCond *cond;
    int result;

    cond = (SysCond *)sys_malloc(sizeof(SysCond));
    if (cond == NULL) {
        sys_seterr(ERR_IO);
        return NULL;
    }

    result = pthread_cond_init(&cond->impl, NULL);
    if (result != 0) {
        sys_free(cond);
        sys_seterr(ERR_IO);
        return NULL;
    }

    return cond;
}

int
sys_cond_wait(SysCond *cond, SysMutex *mutex)
{
    if (cond == NULL || mutex == NULL) {
        sys_seterr(ERR_IO);
        return -1;
    }
    return sys_pthread_fail(pthread_cond_wait(&cond->impl, &mutex->impl));
}

int
sys_cond_signal(SysCond *cond)
{
    if (cond == NULL) {
        sys_seterr(ERR_IO);
        return -1;
    }
    return sys_pthread_fail(pthread_cond_signal(&cond->impl));
}

int
sys_cond_broadcast(SysCond *cond)
{
    if (cond == NULL) {
        sys_seterr(ERR_IO);
        return -1;
    }
    return sys_pthread_fail(pthread_cond_broadcast(&cond->impl));
}

void
sys_cond_destroy(SysCond *cond)
{
    if (cond == NULL) {
        return;
    }
    pthread_cond_destroy(&cond->impl);
    sys_free(cond);
}

int
sys_thread_create(SysThreadFn fn, void *arg, SysThread **out)
{
    SysThread *thread;
    int result;

    if (fn == NULL || out == NULL) {
        sys_seterr(ERR_IO);
        return -1;
    }

    thread = (SysThread *)sys_malloc(sizeof(SysThread));
    if (thread == NULL) {
        sys_seterr(ERR_IO);
        return -1;
    }

    result = pthread_create(&thread->impl, NULL, fn, arg);
    if (result != 0) {
        sys_free(thread);
        sys_seterr(ERR_IO);
        return -1;
    }

    *out = thread;
    return 0;
}

int
sys_thread_join(SysThread *thread, void **retval)
{
    int result;

    if (thread == NULL) {
        sys_seterr(ERR_IO);
        return -1;
    }

    result = pthread_join(thread->impl, retval);
    sys_free(thread);
    return sys_pthread_fail(result);
}
