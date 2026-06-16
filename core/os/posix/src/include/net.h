#ifndef NET_H
#define NET_H

#include "sysdeps.h"

enum {
    ODIN_AF_UNSPEC = 0,
    ODIN_AF_UNIX   = 1,
    ODIN_AF_INET   = 2
};

enum {
    ODIN_SOCK_STREAM = 1,
    ODIN_SOCK_DGRAM  = 2
};

enum {
    ODIN_SHUT_RD   = 0,
    ODIN_SHUT_WR   = 1,
    ODIN_SHUT_RDWR = 2
};

#define AF_UNSPEC ODIN_AF_UNSPEC
#define AF_UNIX   ODIN_AF_UNIX
#define AF_INET   ODIN_AF_INET

#define SOCK_STREAM ODIN_SOCK_STREAM
#define SOCK_DGRAM  ODIN_SOCK_DGRAM

#define SHUT_RD   ODIN_SHUT_RD
#define SHUT_WR   ODIN_SHUT_WR
#define SHUT_RDWR ODIN_SHUT_RDWR

#define SYS_MSG_NONE 0

#define SYS_SUN_PATH_SIZE 108

typedef struct SysSockAddrIn {
    uint16_t sin_family;
    uint16_t sin_port;
    uint32_t sin_addr;
    unsigned char sin_zero[8];
} SysSockAddrIn;

typedef struct SysSockAddrUn {
    uint16_t sun_family;
    char sun_path[SYS_SUN_PATH_SIZE];
} SysSockAddrUn;

int sys_socket(int domain, int type, int protocol);
int sys_socketpair(int domain, int type, int protocol, int sv[2]);
int sys_bind(int sockfd, const void *addr, unsigned int addrlen);
int sys_connect(int sockfd, const void *addr, unsigned int addrlen);
int sys_listen(int sockfd, int backlog);
int sys_accept(int sockfd, void *addr, unsigned int *addrlen);
long sys_send(int sockfd, const void *buf, long len, int flags);
long sys_recv(int sockfd, void *buf, long len, int flags);
int sys_shutdown(int sockfd, int how);

uint16_t sys_htons(uint16_t hostshort);
uint32_t sys_htonl(uint32_t hostlong);

#endif
