#include "common.h"
#include "sysdeps.h"
#include "net.h"
#include "process.h"
#include "sys_thread.h"
#include "mem.h"

static fd_t g_listen;

static int
parse_port(char *s, uint32 *port)
{
	char *e;
	ulong v;

	if(s == nil || port == nil) {
		sys_seterr(ERR_IO);
		return -1;
	}
	v = strtoul(s, &e, 10);
	if(e == s || *e != '\0' || v > 65535) {
		sys_seterr(ERR_IO);
		return -1;
	}
	*port = (uint32)v;
	return 0;
}

static int
run_echo_client(char *host, uint32 port)
{
	SysSockAddr addr;
	fd_t client;
	char msg[32];
	char buf[64];
	long n;
	int failed;

	failed = 0;
	client = sys_socket(ODIN_AF_INET, ODIN_SOCK_STREAM, 0);
	if(client < 0) {
		print("FAIL: client socket\n");
		return 1;
	}

	if(host != nil) {
		if(sys_sockaddr_in(&addr, host, port) != 0) {
			print("FAIL: sockaddr_in\n");
			failed = 1;
		}
	} else if(sys_sockaddr_local_tcp(&addr, port) != 0) {
		print("FAIL: sockaddr local\n");
		failed = 1;
	}

	if(!failed) {
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
	}

	sys_socket_close(client);
	return failed;
}

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

static int
run_local_test(void)
{
	int failed;
	SysSockAddr addr;
	SysThread *thread;
	uint32 port;

	failed = 0;
	port = (uint32)(40000 + (sys_getpid() % 20000));
	print("net test: local port=%lud\n", (ulong)port);

	g_listen = sys_socket(ODIN_AF_INET, ODIN_SOCK_STREAM, 0);
	if(g_listen < 0) {
		print("FAIL: socket\n");
		return 1;
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
		} else {
			print("net test: announced dir %s\n", sys_socket_dir(g_listen));
		}
	}

	if(!failed) {
		print("net test: listen...\n");
		if(sys_listen(g_listen, 1) != 0) {
			print("FAIL: listen\n");
			failed = 1;
		} else {
			print("net test: listen ok\n");
		}
	}

	if(!failed) {
		print("net test: spawning server proc...\n");
		if(sys_thread_create(server_thread, nil, &thread) != 0) {
			print("FAIL: thread_create\n");
			failed = 1;
		} else {
			sleep(200);
			failed = run_echo_client(nil, port);
		}
	}

	if(!failed && sys_thread_join(thread, nil) != 0) {
		print("FAIL: thread_join\n");
		failed = 1;
	}

	sys_socket_close(g_listen);
	return failed;
}

void
threadmain(int argc, char **argv)
{
	uint32 port;
	int failed;

	failed = 0;

	if(argc == 3) {
		if(parse_port(argv[2], &port) != 0) {
			print("usage: test_net [host port]\n");
			exits("usage");
		}
		print("net test: remote host=%s port=%lud\n", argv[1], (ulong)port);
		failed = run_echo_client(argv[1], port);
	} else if(argc == 1) {
		failed = run_local_test();
	} else {
		print("usage: test_net [host port]\n");
		exits("usage");
	}

	if(!failed)
		print("\nAll network tests passed!\n");

	exits(failed ? "fail" : nil);
}
