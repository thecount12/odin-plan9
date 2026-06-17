#include "common.h"
#include "sysdeps.h"
#include "net.h"
#include "process.h"
#include "sys_thread.h"
#include "mem.h"

static fd_t g_listen;

static void *
server_thread(void *arg)
{
	fd_t conn;
	char buf[64];
	long n;

	USED(arg);

	print("net test: server waiting for accept...\n");
	conn = sys_accept(g_listen, nil);
	if(conn < 0) {
		print("net test: server accept failed\n");
		return nil;
	}

	n = sys_recv(conn, buf, sizeof(buf) - 1, 0);
	if(n > 0)
		sys_send(conn, buf, n, 0);
	sys_socket_close(conn);
	print("net test: server done\n");
	return nil;
}

void
threadmain(int argc, char **argv)
{
	int failed;
	SysSockAddr addr;
	SysThread *thread;
	fd_t client;
	char msg[32];
	char buf[64];
	long n;
	uint32 port;

	USED(argc);
	USED(argv);

	failed = 0;
	port = (uint32)(40000 + (sys_getpid() % 20000));
	print("net test: port=%lud\n", (ulong)port);

	g_listen = sys_socket(ODIN_AF_INET, ODIN_SOCK_STREAM, 0);
	if(g_listen < 0) {
		print("FAIL: socket\n");
		exits("fail");
	}

	if(sys_socket_set_reuseaddr(g_listen, 1) != 0) {
		print("FAIL: set_reuseaddr\n");
		failed = 1;
	}

	if(!failed && sys_sockaddr_listen_tcp(&addr, port) != 0) {
		print("FAIL: sockaddr listen\n");
		failed = 1;
	}

	if(!failed) {
		print("net test: announce %s\n", (char *)addr.storage);
		if(sys_bind(g_listen, &addr) != 0) {
			print("FAIL: bind\n");
			failed = 1;
		}
	}

	if(!failed) {
		print("net test: listen...\n");
		if(sys_listen(g_listen, 1) != 0) {
			print("FAIL: listen\n");
			failed = 1;
		}
	}

	if(!failed) {
		print("net test: spawning server proc...\n");
		if(sys_thread_create(server_thread, nil, &thread) != 0) {
			print("FAIL: thread_create\n");
			failed = 1;
		} else {
			/* let the server proc reach accept() before we dial */
			sleep(200);
		}
	}

	if(!failed) {
		client = sys_socket(ODIN_AF_INET, ODIN_SOCK_STREAM, 0);
		if(client < 0) {
			print("FAIL: client socket\n");
			failed = 1;
		} else if(sys_sockaddr_local_tcp(&addr, port) != 0) {
			print("FAIL: sockaddr local\n");
			failed = 1;
		} else {
			print("net test: dial %s\n", (char *)addr.storage);
			if(sys_connect(client, &addr) != 0) {
				print("FAIL: connect\n");
				failed = 1;
			} else {
				sys_memcpy(msg, "odin_net", 9);
				if(sys_send(client, msg, 8, 0) != 8) {
					print("FAIL: send\n");
					failed = 1;
				} else {
					n = sys_recv(client, buf, sizeof(buf) - 1, 0);
					if(n != 8 || sys_memcmp(buf, "odin_net", 8) != 0) {
						print("FAIL: recv echo (n=%ld)\n", n);
						failed = 1;
					} else {
						print("PASS: tcp connect/send/recv echo\n");
					}
				}
			}
			sys_socket_close(client);
		}
	}

	if(!failed && sys_thread_join(thread, nil) != 0) {
		print("FAIL: thread_join\n");
		failed = 1;
	}

	sys_socket_close(g_listen);

	if(!failed)
		print("\nAll network tests passed!\n");

	exits(failed ? "fail" : nil);
}
