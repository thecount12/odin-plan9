/**
 * @file thread.h
 *
 * Threading primitives for Plan 9.
 * Implements similar functionality to POSIX threads using:
 * - procrfork() for thread creation
 * - channels ( Chan) for synchronization
 */

#ifndef _THREAD_H_
#define _THREAD_H_

#include <u.h>
#include <libc.h>

typedef struct {
    // ... channel-based thread implementation ...
} sys_thread_t;

// Exported functions
int sys_thread_create(sys_thread_t *thread, void *(*start_routine)(void*), void *arg);
int sys_thread_join(sys_thread_t thread, void **retval);

#endif // _THREAD_H_
