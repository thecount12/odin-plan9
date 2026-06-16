#include <stdio.h>
#include "include/net.h"
#include "include/filesys.h"
#include "include/mem.h"

static int test_socketpair(void) {
    int sv[2];
    const char *msg = "socketpair ping";
    char buf[32];
    long sent;
    long got;

    if (sys_socketpair(ODIN_AF_UNIX, ODIN_SOCK_STREAM, 0, sv) != 0) {
        printf("FAIL: socketpair\n");
        return 1;
    }

    sent = sys_send(sv[0], msg, (long)sys_strlen(msg), SYS_MSG_NONE);
    if (sent != (long)sys_strlen(msg)) {
        printf("FAIL: socketpair send\n");
        sys_close(sv[0]);
        sys_close(sv[1]);
        return 1;
    }

    got = sys_recv(sv[1], buf, (long)sizeof(buf) - 1, SYS_MSG_NONE);
    if (got != (long)sys_strlen(msg)) {
        printf("FAIL: socketpair recv\n");
        sys_close(sv[0]);
        sys_close(sv[1]);
        return 1;
    }

    buf[got] = '\0';
    if (sys_memcmp(buf, msg, (size_t)sys_strlen(msg) + 1) != 0) {
        printf("FAIL: socketpair payload\n");
        sys_close(sv[0]);
        sys_close(sv[1]);
        return 1;
    }

    sys_shutdown(sv[0], ODIN_SHUT_WR);
    sys_close(sv[0]);
    sys_close(sv[1]);
    printf("PASS: socketpair send/recv\n");
    return 0;
}

static int test_unix_stream(void) {
    const char *path = "/tmp/odin_posix_net_test.sock";
    const char *msg = "unix stream hello";
    SysSockAddrUn addr;
    int server;
    int client;
    int peer;
    char buf[32];
    long got;

    sys_unlink(path);

    server = sys_socket(ODIN_AF_UNIX, ODIN_SOCK_STREAM, 0);
    client = sys_socket(ODIN_AF_UNIX, ODIN_SOCK_STREAM, 0);
    if (server < 0 || client < 0) {
        printf("FAIL: unix socket create\n");
        return 1;
    }

    sys_memset(&addr, 0, sizeof(addr));
    addr.sun_family = ODIN_AF_UNIX;
    sys_memcpy(addr.sun_path, path, (size_t)sys_strlen(path) + 1);

    if (sys_bind(server, &addr, (unsigned int)sizeof(addr)) != 0) {
        printf("FAIL: unix bind\n");
        sys_close(server);
        sys_close(client);
        return 1;
    }

    if (sys_listen(server, 4) != 0) {
        printf("FAIL: unix listen\n");
        sys_close(server);
        sys_close(client);
        return 1;
    }

    if (sys_connect(client, &addr, (unsigned int)sizeof(addr)) != 0) {
        printf("FAIL: unix connect\n");
        sys_close(server);
        sys_close(client);
        return 1;
    }

    peer = sys_accept(server, NULL, NULL);
    if (peer < 0) {
        printf("FAIL: unix accept\n");
        sys_close(server);
        sys_close(client);
        return 1;
    }

    if (sys_send(client, msg, (long)sys_strlen(msg), SYS_MSG_NONE) != (long)sys_strlen(msg)) {
        printf("FAIL: unix send\n");
        sys_close(server);
        sys_close(client);
        sys_close(peer);
        return 1;
    }

    got = sys_recv(peer, buf, (long)sizeof(buf) - 1, SYS_MSG_NONE);
    if (got != (long)sys_strlen(msg)) {
        printf("FAIL: unix recv\n");
        sys_close(server);
        sys_close(client);
        sys_close(peer);
        return 1;
    }

    buf[got] = '\0';
    if (sys_memcmp(buf, msg, (size_t)sys_strlen(msg) + 1) != 0) {
        printf("FAIL: unix payload\n");
        sys_close(server);
        sys_close(client);
        sys_close(peer);
        return 1;
    }

    sys_close(server);
    sys_close(client);
    sys_close(peer);
    sys_unlink(path);
    printf("PASS: unix stream bind/listen/connect/accept\n");
    return 0;
}

int main(void) {
    int failed = 0;

    if (test_socketpair() != 0) {
        failed = 1;
    }
    if (test_unix_stream() != 0) {
        failed = 1;
    }

    if (!failed) {
        printf("\nAll net tests passed!\n");
    }

    return failed ? 1 : 0;
}
