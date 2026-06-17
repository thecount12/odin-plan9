#ifndef THREAD_H
#define THREAD_H

typedef struct SysMutex SysMutex;
typedef struct SysCond SysCond;
typedef struct SysThread SysThread;

typedef void *(*SysThreadFn)(void *arg);

SysMutex *sys_mutex_create(void);
int sys_mutex_lock(SysMutex *mutex);
int sys_mutex_unlock(SysMutex *mutex);
void sys_mutex_destroy(SysMutex *mutex);

SysCond *sys_cond_create(void);
int sys_cond_wait(SysCond *cond, SysMutex *mutex);
int sys_cond_signal(SysCond *cond);
int sys_cond_broadcast(SysCond *cond);
void sys_cond_destroy(SysCond *cond);

int sys_thread_create(SysThreadFn fn, void *arg, SysThread **out);
int sys_thread_join(SysThread *thread, void **retval);

#endif
