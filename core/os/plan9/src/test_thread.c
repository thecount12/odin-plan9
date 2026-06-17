#include "common.h"
#include "sysdeps.h"
#include "sys_thread.h"
#include "mem.h"

static SysMutex *g_mutex;
static SysCond *g_cond;
static int g_ready;
static int g_count;

static void *
worker_fn(void *arg)
{
	USED(arg);

	if(sys_mutex_lock(g_mutex) != 0)
		return nil;

	while(!g_ready) {
		if(sys_cond_wait(g_cond, g_mutex) != 0) {
			sys_mutex_unlock(g_mutex);
			return nil;
		}
	}

	g_count += 1;
	sys_mutex_unlock(g_mutex);
	return nil;
}

void
threadmain(int argc, char **argv)
{
	int failed;
	SysThread *thread;

	USED(argc);
	USED(argv);
	failed = 0;
	g_count = 0;
	g_ready = 0;
	g_mutex = sys_mutex_create();
	g_cond = sys_cond_create();

	if(g_mutex == nil || g_cond == nil) {
		print("FAIL: create mutex/cond\n");
		exits("fail");
	}

	if(sys_thread_create(worker_fn, nil, &thread) != 0) {
		print("FAIL: thread_create\n");
		failed = 1;
	} else {
		if(sys_mutex_lock(g_mutex) != 0) {
			print("FAIL: mutex_lock (main)\n");
			failed = 1;
		} else {
			g_ready = 1;
			if(sys_cond_signal(g_cond) != 0) {
				print("FAIL: cond_signal\n");
				failed = 1;
			}
			sys_mutex_unlock(g_mutex);
		}

		if(!failed && sys_thread_join(thread, nil) != 0) {
			print("FAIL: thread_join\n");
			failed = 1;
		} else if(!failed && g_count != 1) {
			print("FAIL: worker count=%d expected 1\n", g_count);
			failed = 1;
		} else if(!failed) {
			print("PASS: thread create/join with cond/mutex\n");
		}
	}

	sys_cond_destroy(g_cond);
	sys_mutex_destroy(g_mutex);

	if(!failed)
		print("\nAll thread tests passed!\n");

	exits(failed ? "fail" : nil);
}
