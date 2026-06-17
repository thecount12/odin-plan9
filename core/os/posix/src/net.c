#include "include/net.h"
#include "include/mem.h"
#include "include/sysdeps.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

int
sys_sockaddr_in(SysSockAddr *addr, const char *host, uint32 port)
{
    struct sockaddr_in *in;
    unsigned long ip;

    if (addr == NULL || host == NULL) {
        sys_seterr(ERR_IO);
        return -1;
    }

    if (sizeof(struct sockaddr_in) > SYS_SOCKADDR_MAX) {
        sys_seterr(ERR_IO);
        return -1;
    }

    sys_memset(addr, 0, sizeof(SysSockAddr));
    in = (struct sockaddr_in *)addr->storage;
    in->sin_family = AF_INET;
    in->sin_port = htons((unsigned short)port);

    ip = inet_addr(host);
    if (ip == (unsigned long)INADDR_NONE) {
        sys_seterr(ERR_IO);
        return -1;
    }
    in->sin_addr.s_addr = (in_addr_t)ip;
    addr->len = (ulong)sizeof(struct sockaddr_in);
    return 0;
}

fd_t
sys_socket(int domain, int type, int protocol)
{
    int fd;

    fd = socket(domain, type, protocol);
    if (fd < 0) {
        sys_seterr_posix();
        return (fd_t)-1;
    }
    return (fd_t)fd;
}

int
sys_socket_set_reuseaddr(fd_t sock, int on)
{
    int value;

    if (sock < 0) {
        sys_seterr(ERR_IO);
        return -1;
    }

    value = on ? 1 : 0;
    if (setsockopt((int)sock, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value)) < 0) {
        return sys_seterr_posix();
    }
    return 0;
}

int
sys_bind(fd_t sock, const SysSockAddr *addr)
{
    if (sock < 0 || addr == NULL || addr->len == 0) {
        sys_seterr(ERR_IO);
        return -1;
    }
    if (bind((int)sock, (struct sockaddr *)addr->storage, (socklen_t)addr->len) < 0) {
        return sys_seterr_posix();
    }
    return 0;
}

int
sys_listen(fd_t sock, int backlog)
{
    if (sock < 0) {
        sys_seterr(ERR_IO);
        return -1;
    }
    if (listen((int)sock, backlog) < 0) {
        return sys_seterr_posix();
    }
    return 0;
}

fd_t
sys_accept(fd_t sock, SysSockAddr *addr)
{
    int result;
    socklen_t addrlen;

    if (sock < 0) {
        sys_seterr(ERR_IO);
        return (fd_t)-1;
    }

    if (addr == NULL) {
        result = accept((int)sock, NULL, NULL);
    } else {
        addrlen = (socklen_t)addr->len;
        result = accept((int)sock, (struct sockaddr *)addr->storage, &addrlen);
        addr->len = (ulong)addrlen;
    }

    if (result < 0) {
        sys_seterr_posix();
        return (fd_t)-1;
    }
    return (fd_t)result;
}

int
sys_connect(fd_t sock, const SysSockAddr *addr)
{
    if (sock < 0 || addr == NULL || addr->len == 0) {
        sys_seterr(ERR_IO);
        return -1;
    }
    if (connect((int)sock, (struct sockaddr *)addr->storage, (socklen_t)addr->len) < 0) {
        return sys_seterr_posix();
    }
    return 0;
}

long
sys_send(fd_t sock, void *buf, long len, int flags)
{
    ssize_t n;

    if (sock < 0 || buf == NULL || len < 0) {
        sys_seterr(ERR_IO);
        return -1;
    }

    n = send((int)sock, buf, (size_t)len, flags);
    if (n < 0) {
        sys_seterr_posix();
        return -1;
    }
    return (long)n;
}

long
sys_recv(fd_t sock, void *buf, long len, int flags)
{
    ssize_t n;

    if (sock < 0 || buf == NULL || len < 0) {
        sys_seterr(ERR_IO);
        return -1;
    }

    n = recv((int)sock, buf, (size_t)len, flags);
    if (n < 0) {
        sys_seterr_posix();
        return -1;
    }
    return (long)n;
}
