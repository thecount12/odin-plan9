#include "include/net.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#define SYS_SOCKADDR_UN_HAS_LEN 1
#endif

static int odin_to_posix_af(int domain) {
    switch (domain) {
        case ODIN_AF_UNIX: return AF_UNIX;
        case ODIN_AF_INET: return AF_INET;
        default: return domain;
    }
}

static int odin_to_posix_sock(int type) {
    switch (type) {
        case ODIN_SOCK_STREAM: return SOCK_STREAM;
        case ODIN_SOCK_DGRAM:  return SOCK_DGRAM;
        default: return type;
    }
}

static int odin_to_posix_shut(int how) {
    switch (how) {
        case ODIN_SHUT_RD:   return SHUT_RD;
        case ODIN_SHUT_WR:   return SHUT_WR;
        case ODIN_SHUT_RDWR: return SHUT_RDWR;
        default: return how;
    }
}

static socklen_t sockaddr_un_size(const struct sockaddr_un *addr) {
#ifdef SYS_SOCKADDR_UN_HAS_LEN
    if (addr->sun_len != 0) {
        return (socklen_t)addr->sun_len;
    }
    return (socklen_t)(offsetof(struct sockaddr_un, sun_path) + strlen(addr->sun_path));
#else
    return (socklen_t)sizeof(*addr);
#endif
}

static void convert_sockaddr_un(struct sockaddr_un *dst, const SysSockAddrUn *src) {
    memset(dst, 0, sizeof(*dst));
    dst->sun_family = (sa_family_t)odin_to_posix_af((int)src->sun_family);
    strncpy(dst->sun_path, src->sun_path, sizeof(dst->sun_path) - 1);
    dst->sun_path[sizeof(dst->sun_path) - 1] = '\0';
#ifdef SYS_SOCKADDR_UN_HAS_LEN
    dst->sun_len = (unsigned char)sockaddr_un_size(dst);
#endif
}

static void convert_sockaddr_in(struct sockaddr_in *dst, const SysSockAddrIn *src) {
    memset(dst, 0, sizeof(*dst));
    dst->sin_family = (sa_family_t)odin_to_posix_af((int)src->sin_family);
    dst->sin_port = src->sin_port;
    dst->sin_addr.s_addr = src->sin_addr;
}

int sys_socket(int domain, int type, int protocol) {
    int fd;

    fd = (int)socket(odin_to_posix_af(domain), odin_to_posix_sock(type), protocol);
    if (fd < 0) {
        return sys_seterr_posix();
    }
    return fd;
}

int sys_socketpair(int domain, int type, int protocol, int sv[2]) {
    int result;

    if (sv == NULL) {
        sys_seterr(ERR_IO);
        return -1;
    }

    result = socketpair(odin_to_posix_af(domain), odin_to_posix_sock(type), protocol, sv);
    if (result != 0) {
        return sys_seterr_posix();
    }
    return 0;
}

int sys_bind(int sockfd, const void *addr, unsigned int addrlen) {
    struct sockaddr_un un_addr;
    struct sockaddr_in in_addr;
    const SysSockAddrUn *src_un;
    const SysSockAddrIn *src_in;
    int result;

    if (addr == NULL) {
        sys_seterr(ERR_IO);
        return -1;
    }

    src_un = (const SysSockAddrUn *)addr;
    if (src_un->sun_family == ODIN_AF_UNIX) {
        convert_sockaddr_un(&un_addr, src_un);
        result = bind(sockfd, (struct sockaddr *)&un_addr, sockaddr_un_size(&un_addr));
    } else if (src_un->sun_family == ODIN_AF_INET && addrlen >= sizeof(SysSockAddrIn)) {
        src_in = (const SysSockAddrIn *)addr;
        convert_sockaddr_in(&in_addr, src_in);
        result = bind(sockfd, (struct sockaddr *)&in_addr, (socklen_t)sizeof(in_addr));
    } else {
        sys_seterr(ERR_IO);
        return -1;
    }

    if (result != 0) {
        return sys_seterr_posix();
    }
    return 0;
}

int sys_connect(int sockfd, const void *addr, unsigned int addrlen) {
    struct sockaddr_un un_addr;
    struct sockaddr_in in_addr;
    const SysSockAddrUn *src_un;
    const SysSockAddrIn *src_in;
    int result;

    if (addr == NULL) {
        sys_seterr(ERR_IO);
        return -1;
    }

    src_un = (const SysSockAddrUn *)addr;
    if (src_un->sun_family == ODIN_AF_UNIX) {
        convert_sockaddr_un(&un_addr, src_un);
        result = connect(sockfd, (struct sockaddr *)&un_addr, sockaddr_un_size(&un_addr));
    } else if (src_un->sun_family == ODIN_AF_INET && addrlen >= sizeof(SysSockAddrIn)) {
        src_in = (const SysSockAddrIn *)addr;
        convert_sockaddr_in(&in_addr, src_in);
        result = connect(sockfd, (struct sockaddr *)&in_addr, (socklen_t)sizeof(in_addr));
    } else {
        sys_seterr(ERR_IO);
        return -1;
    }

    if (result != 0) {
        return sys_seterr_posix();
    }
    return 0;
}

int sys_listen(int sockfd, int backlog) {
    int result;

    result = listen(sockfd, backlog);
    if (result != 0) {
        return sys_seterr_posix();
    }
    return 0;
}

int sys_accept(int sockfd, void *addr, unsigned int *addrlen) {
    struct sockaddr_un un_addr;
    struct sockaddr_in in_addr;
    socklen_t len;
    int fd;

    if (addr != NULL && addrlen != NULL && *addrlen >= sizeof(SysSockAddrUn)) {
        SysSockAddrUn *out_un;
        SysSockAddrIn *out_in;

        out_un = (SysSockAddrUn *)addr;
        if (out_un->sun_family == ODIN_AF_UNIX) {
            len = (socklen_t)sizeof(un_addr);
            fd = (int)accept(sockfd, (struct sockaddr *)&un_addr, &len);
            if (fd < 0) {
                return sys_seterr_posix();
            }
            out_un->sun_family = ODIN_AF_UNIX;
            strncpy(out_un->sun_path, un_addr.sun_path, sizeof(out_un->sun_path) - 1);
            out_un->sun_path[sizeof(out_un->sun_path) - 1] = '\0';
            *addrlen = (unsigned int)sizeof(SysSockAddrUn);
            return fd;
        }

        if (out_un->sun_family == ODIN_AF_INET && *addrlen >= sizeof(SysSockAddrIn)) {
            out_in = (SysSockAddrIn *)addr;
            len = (socklen_t)sizeof(in_addr);
            fd = (int)accept(sockfd, (struct sockaddr *)&in_addr, &len);
            if (fd < 0) {
                return sys_seterr_posix();
            }
            out_in->sin_family = ODIN_AF_INET;
            out_in->sin_port = in_addr.sin_port;
            out_in->sin_addr = in_addr.sin_addr.s_addr;
            *addrlen = (unsigned int)sizeof(SysSockAddrIn);
            return fd;
        }
    }

    fd = (int)accept(sockfd, NULL, NULL);
    if (fd < 0) {
        return sys_seterr_posix();
    }
    return fd;
}

long sys_send(int sockfd, const void *buf, long len, int flags) {
    ssize_t n;

    if (buf == NULL || len < 0) {
        sys_seterr(ERR_IO);
        return -1;
    }

    n = send(sockfd, buf, (size_t)len, flags);
    if (n < 0) {
        sys_seterr_posix();
        return -1;
    }
    return (long)n;
}

long sys_recv(int sockfd, void *buf, long len, int flags) {
    ssize_t n;

    if (buf == NULL || len < 0) {
        sys_seterr(ERR_IO);
        return -1;
    }

    n = recv(sockfd, buf, (size_t)len, flags);
    if (n < 0) {
        sys_seterr_posix();
        return -1;
    }
    return (long)n;
}

int sys_shutdown(int sockfd, int how) {
    int result;

    result = shutdown(sockfd, odin_to_posix_shut(how));
    if (result != 0) {
        return sys_seterr_posix();
    }
    return 0;
}

uint16_t sys_htons(uint16_t hostshort) {
    return (uint16_t)(((hostshort & 0xffU) << 8) | ((hostshort >> 8) & 0xffU));
}

uint32_t sys_htonl(uint32_t hostlong) {
    return ((hostlong & 0x000000ffUL) << 24) |
           ((hostlong & 0x0000ff00UL) << 8)  |
           ((hostlong & 0x00ff0000UL) >> 8)  |
           ((hostlong & 0xff000000UL) >> 24);
}
