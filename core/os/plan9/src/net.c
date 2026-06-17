#include "common.h"
#include "sysdeps.h"
#include "net.h"
#include "mem.h"

enum {
	SOCK_FREE = 0,
	SOCK_SERVER,
	SOCK_CONN
};

#define NET_SLOTS 16

typedef struct {
	int state;
	char adir[40];
	int lfd;
	int cfd;
} NetSlot;

static NetSlot net_slot[NET_SLOTS];

static int
net_slot_valid(fd_t sock)
{
	return sock >= 0 && sock < NET_SLOTS && net_slot[sock].state != SOCK_FREE;
}

static char *
dial_port(char *dial)
{
	char *p;

	p = strrchr(dial, '!');
	if(p == nil)
		return nil;
	return p + 1;
}

int
sys_sockaddr_in(SysSockAddr *addr, char *host, uint32 port)
{
	if(addr == nil || host == nil) {
		sys_seterr(ERR_IO);
		return -1;
	}
	snprint((char *)addr->storage, SYS_SOCKADDR_MAX, "tcp!%s!%lud", host, (ulong)port);
	addr->len = strlen((char *)addr->storage);
	return 0;
}

fd_t
sys_socket(int domain, int type, int protocol)
{
	int i;

	USED(domain);
	USED(type);
	USED(protocol);

	for(i = 0; i < NET_SLOTS; i++) {
		if(net_slot[i].state == SOCK_FREE) {
			memset(&net_slot[i], 0, sizeof(NetSlot));
			net_slot[i].state = SOCK_SERVER;
			net_slot[i].lfd = -1;
			net_slot[i].cfd = -1;
			return (fd_t)i;
		}
	}
	sys_seterr(ERR_IO);
	return -1;
}

int
sys_socket_set_reuseaddr(fd_t sock, int on)
{
	USED(sock);
	USED(on);
	return 0;
}

int
sys_bind(fd_t sock, SysSockAddr *addr)
{
	NetSlot *n;
	char ann[SYS_SOCKADDR_MAX];
	char *port;

	if(!net_slot_valid(sock) || addr == nil) {
		sys_seterr(ERR_IO);
		return -1;
	}
	n = &net_slot[sock];
	port = dial_port((char *)addr->storage);
	if(port == nil) {
		sys_seterr(ERR_IO);
		return -1;
	}
	snprint(ann, sizeof ann, "tcp!*!%s", port);
	if(announce(ann, n->adir) < 0)
		return sys_seterr_plan9();
	return 0;
}

int
sys_listen(fd_t sock, int backlog)
{
	NetSlot *n;

	USED(backlog);
	if(!net_slot_valid(sock)) {
		sys_seterr(ERR_IO);
		return -1;
	}
	n = &net_slot[sock];
	n->lfd = listen(n->adir, "tcp");
	if(n->lfd < 0)
		return sys_seterr_plan9();
	return 0;
}

fd_t
sys_accept(fd_t sock, SysSockAddr *addr)
{
	NetSlot *n;
	int cfd;
	int i;

	USED(addr);
	if(!net_slot_valid(sock)) {
		sys_seterr(ERR_IO);
		return -1;
	}
	n = &net_slot[sock];
	if(n->lfd < 0) {
		sys_seterr(ERR_IO);
		return -1;
	}
	cfd = accept(n->lfd, nil);
	if(cfd < 0)
		return sys_seterr_plan9();
	for(i = 0; i < NET_SLOTS; i++) {
		if(net_slot[i].state == SOCK_FREE) {
			memset(&net_slot[i], 0, sizeof(NetSlot));
			net_slot[i].state = SOCK_CONN;
			net_slot[i].cfd = cfd;
			return (fd_t)i;
		}
	}
	close(cfd);
	sys_seterr(ERR_IO);
	return -1;
}

int
sys_connect(fd_t sock, SysSockAddr *addr)
{
	NetSlot *n;
	int cfd;

	if(!net_slot_valid(sock) || addr == nil) {
		sys_seterr(ERR_IO);
		return -1;
	}
	n = &net_slot[sock];
	cfd = dial((char *)addr->storage, nil, nil, nil);
	if(cfd < 0)
		return sys_seterr_plan9();
	n->cfd = cfd;
	n->state = SOCK_CONN;
	return 0;
}

static int
net_io_fd(fd_t sock)
{
	if(!net_slot_valid(sock))
		return -1;
	if(net_slot[sock].state == SOCK_CONN && net_slot[sock].cfd >= 0)
		return net_slot[sock].cfd;
	return -1;
}

long
sys_send(fd_t sock, void *buf, long len, int flags)
{
	int fd;
	long n;

	USED(flags);
	if(buf == nil || len < 0) {
		sys_seterr(ERR_IO);
		return -1;
	}
	fd = net_io_fd(sock);
	if(fd < 0) {
		sys_seterr(ERR_IO);
		return -1;
	}
	n = write(fd, buf, len);
	if(n < 0)
		return sys_seterr_plan9();
	return n;
}

long
sys_recv(fd_t sock, void *buf, long len, int flags)
{
	int fd;
	long n;

	USED(flags);
	if(buf == nil || len < 0) {
		sys_seterr(ERR_IO);
		return -1;
	}
	fd = net_io_fd(sock);
	if(fd < 0) {
		sys_seterr(ERR_IO);
		return -1;
	}
	n = read(fd, buf, len);
	if(n < 0)
		return sys_seterr_plan9();
	return n;
}

void
sys_socket_close(fd_t sock)
{
	NetSlot *n;

	if(sock < 0 || sock >= NET_SLOTS)
		return;
	n = &net_slot[sock];
	if(n->cfd >= 0)
		close(n->cfd);
	if(n->lfd >= 0)
		close(n->lfd);
	memset(n, 0, sizeof(NetSlot));
}
