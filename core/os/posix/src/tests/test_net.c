#include <stdio.h>
#include "include/net.h"
#include "include/filesys.h"
#include "include/process.h"
#include "include/thread.h"
#include "include/mem.h"

#define TEST_HOST "127.0.0.1"

static fd_t g_listen;

static void *
server_thread(void *arg)
{
    fd_t conn;
    char buf[64];
    long n;

    (void)arg;

    conn = sys_accept(g_listen, NULL);
    if (conn < 0) {
        return NULL;
    }

    n = sys_recv(conn, buf, (long)sizeof(buf) - 1, 0);
    if (n > 0) {
        sys_send(conn, buf, n, 0);
    }
    sys_close(conn);
    return NULL;
}

int
main(void)
{
    int failed;
    SysSockAddr addr;
    SysThread *thread;
    fd_t client;
    char msg[32];
    char buf[64];
    long n;
    uint32 port;

    failed = 0;
    port = (uint32)(40000 + (sys_getpid() % 20000));

    g_listen = sys_socket(ODIN_AF_INET, ODIN_SOCK_STREAM, 0);
    if (g_listen < 0) {
        printf("FAIL: socket\n");
        return 1;
    }

    if (sys_socket_set_reuseaddr(g_listen, 1) != 0) {
        printf("FAIL: set_reuseaddr\n");
        failed = 1;
    }

    if (!failed && sys_sockaddr_in(&addr, TEST_HOST, port) != 0) {
        printf("FAIL: sockaddr_in\n");
        failed = 1;
    }

    if (!failed && sys_bind(g_listen, &addr) != 0) {
        printf("FAIL: bind\n");
        failed = 1;
    }

    if (!failed && sys_listen(g_listen, 1) != 0) {
        printf("FAIL: listen\n");
        failed = 1;
    }

    if (!failed && sys_thread_create(server_thread, NULL, &thread) != 0) {
        printf("FAIL: thread_create\n");
        failed = 1;
    }

    if (!failed) {
        client = sys_socket(ODIN_AF_INET, ODIN_SOCK_STREAM, 0);
        if (client < 0) {
            printf("FAIL: client socket\n");
            failed = 1;
        } else {
            if (sys_sockaddr_in(&addr, TEST_HOST, port) != 0 ||
                sys_connect(client, &addr) != 0) {
                printf("FAIL: connect\n");
                failed = 1;
            } else {
                sys_memcpy(msg, "odin_net", 9);
                if (sys_send(client, msg, 8, 0) != 8) {
                    printf("FAIL: send\n");
                    failed = 1;
                } else {
                    n = sys_recv(client, buf, (long)sizeof(buf) - 1, 0);
                    if (n != 8 || sys_memcmp(buf, "odin_net", 8) != 0) {
                        printf("FAIL: recv echo (n=%ld)\n", n);
                        failed = 1;
                    } else {
                        printf("PASS: tcp connect/send/recv echo\n");
                    }
                }
            }
            sys_close(client);
        }
    }

    if (!failed) {
        if (sys_thread_join(thread, NULL) != 0) {
            printf("FAIL: thread_join\n");
            failed = 1;
        }
    }

    sys_close(g_listen);

    if (!failed) {
        printf("\nAll network tests passed!\n");
    }

    return failed ? 1 : 0;
}
