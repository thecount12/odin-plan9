#ifndef THREAD_H
#define THREAD_H

#include "sysdeps.h"
#include <pthread.h>

typedef pthread_t sys_thread_t;
typedef pthread_attr_t sys_thread_attr_t;

typedef pthread_mutex_t sys_mutex_t;
typedef pthread_mutexattr_t sys_mutexattr_t;

typedef pthread_cond_t sys_cond_t;
typedef pthread_condattr_t sys_condattr_t;

int sys_thread_create(sys_thread_t *thread, const sys_thread_attr_t *attr,
                      void *(*start_routine) (void *), void *arg);
int sys_thread_join(sys_thread_t thread, void **retval);

int sys_mutex_init(sys_mutex_t *mutex, const sys_mutexattr_t *attr);
int sys_mutex_lock(sys_mutex_t *mutex);
int sys_mutex_unlock(sys_mutex_t *mutex);
int sys_mutex_destroy(sys_mutex_t *mutex);

int sys_cond_init(sys_cond_t *cond, const sys_condattr_t *attr);
int sys_cond_wait(sys_cond_t *cond, sys_mutex_t *mutex);
int sys_cond_signal(sys_cond_t *cond);
int sys_cond_broadcast(sys_cond_t *cond);
int sys_cond_destroy(sys_cond_t *cond);

#endif
