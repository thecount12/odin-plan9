#ifndef NET_H
#define NET_H

#define SYS_SOCKADDR_MAX 128

typedef struct SysSockAddr {
	unsigned char storage[SYS_SOCKADDR_MAX];
	ulong len;
} SysSockAddr;

enum {
	ODIN_AF_INET = 2,
	ODIN_SOCK_STREAM = 1,
	ODIN_SOCK_DGRAM = 2
};

int sys_sockaddr_in(SysSockAddr *addr, char *host, uint32 port);

fd_t sys_socket(int domain, int type, int protocol);
int sys_socket_set_reuseaddr(fd_t sock, int on);
int sys_bind(fd_t sock, SysSockAddr *addr);
int sys_listen(fd_t sock, int backlog);
fd_t sys_accept(fd_t sock, SysSockAddr *addr);
int sys_connect(fd_t sock, SysSockAddr *addr);
long sys_send(fd_t sock, void *buf, long len, int flags);
long sys_recv(fd_t sock, void *buf, long len, int flags);
void sys_socket_close(fd_t sock);

#endif
