#include "common.h"
#include "sysdeps.h"
#include "sys_thread.h"
#include "mem.h"

#pragma lib "libthread.a"
#include <thread.h>

struct SysMutex {
	Lock lk;
};

struct SysCond {
	Channel *ch;
};

typedef struct ThreadStart ThreadStart;
struct ThreadStart {
	SysThreadFn fn;
	void *arg;
	Channel *done;
};

struct SysThread {
	int pid;
	Channel *done;
};

static void
thread_tramp(void *arg)
{
	ThreadStart *start;

	start = arg;
	start->fn(start->arg);
	sendul(start->done, 1);
	sys_free(start);
}

SysMutex *
sys_mutex_create(void)
{
	SysMutex *mutex;

	mutex = sys_malloc(sizeof(SysMutex));
	if(mutex == nil) {
		sys_seterr(ERR_IO);
		return nil;
	}
	memset(mutex, 0, sizeof(SysMutex));
	return mutex;
}

int
sys_mutex_lock(SysMutex *mutex)
{
	if(mutex == nil) {
		sys_seterr(ERR_IO);
		return -1;
	}
	lock(&mutex->lk);
	return 0;
}

int
sys_mutex_unlock(SysMutex *mutex)
{
	if(mutex == nil) {
		sys_seterr(ERR_IO);
		return -1;
	}
	unlock(&mutex->lk);
	return 0;
}

void
sys_mutex_destroy(SysMutex *mutex)
{
	if(mutex == nil)
		return;
	sys_free(mutex);
}

SysCond *
sys_cond_create(void)
{
	SysCond *cond;

	cond = sys_malloc(sizeof(SysCond));
	if(cond == nil) {
		sys_seterr(ERR_IO);
		return nil;
	}
	cond->ch = chancreate(sizeof(ulong), 8);
	if(cond->ch == nil) {
		sys_free(cond);
		sys_seterr(ERR_IO);
		return nil;
	}
	return cond;
}

int
sys_cond_wait(SysCond *cond, SysMutex *mutex)
{
	if(cond == nil || mutex == nil) {
		sys_seterr(ERR_IO);
		return -1;
	}
	unlock(&mutex->lk);
	recvul(cond->ch);
	lock(&mutex->lk);
	return 0;
}

int
sys_cond_signal(SysCond *cond)
{
	if(cond == nil) {
		sys_seterr(ERR_IO);
		return -1;
	}
	sendul(cond->ch, 1);
	return 0;
}

int
sys_cond_broadcast(SysCond *cond)
{
	if(cond == nil) {
		sys_seterr(ERR_IO);
		return -1;
	}
	sendul(cond->ch, 1);
	return 0;
}

void
sys_cond_destroy(SysCond *cond)
{
	if(cond == nil)
		return;
	sys_free(cond);
}

int
sys_thread_create(SysThreadFn fn, void *arg, SysThread **out)
{
	SysThread *thread;
	ThreadStart *start;

	if(fn == nil || out == nil) {
		sys_seterr(ERR_IO);
		return -1;
	}

	thread = sys_malloc(sizeof(SysThread));
	start = sys_malloc(sizeof(ThreadStart));
	if(thread == nil || start == nil) {
		sys_free(thread);
		sys_free(start);
		sys_seterr(ERR_IO);
		return -1;
	}

	start->fn = fn;
	start->arg = arg;
	start->done = chancreate(sizeof(ulong), 0);
	if(start->done == nil) {
		sys_free(thread);
		sys_free(start);
		sys_seterr(ERR_IO);
		return -1;
	}

	thread->done = start->done;
	thread->pid = proccreate(thread_tramp, start, 8192);
	if(thread->pid < 0) {
		sys_free(thread);
		sys_free(start);
		sys_seterr(ERR_IO);
		return -1;
	}

	*out = thread;
	return 0;
}

int
sys_thread_join(SysThread *thread, void **retval)
{
	USED(retval);
	if(thread == nil) {
		sys_seterr(ERR_IO);
		return -1;
	}
	recvul(thread->done);
	sys_free(thread);
	return 0;
}
