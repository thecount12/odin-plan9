#ifndef SYSDEPS_H
#define SYSDEPS_H

#ifndef USED
#define USED(x) (void)(x)
#endif

typedef unsigned int uint32;

/*
 * 64-bit unsigned integer without <u.h>.
 * Layout matches little-endian uvlong (lo = low 32 bits).
 */
typedef struct {
	ulong lo;
	ulong hi;
} ulonglong;

enum {
	ERR_OK = 0,
	ERR_IO = 5,
	ERR_NOENT = 2,
	ERR_PERM = 1
};

/* Odin-compatible file type bits (POSIX-ish; not Plan 9 DMDIR). */
enum {
	ODIN_S_IFREG = 0100000,
	ODIN_S_IFDIR = 0040000
};

typedef int fd_t;
#define MAX_FD 64

typedef struct Buf {
	unsigned char *data;
	long len;
	long cap;
} Buf;

ulonglong sys_ull(ulong v);
ulonglong sys_ull_from_ptr(void *src);
int sys_ull_eq(ulonglong a, ulonglong b);
int sys_ull_eq_ulong(ulonglong u, ulong v);
int sys_ull_snprint(ulonglong u, char *buf, int n);

int sys_err(void);
void sys_seterr(int err);
int sys_seterr_plan9(void);

#endif
