/**
 * @file mmap.h
 *
 * Memory mapping functions for Plan 9.
 */

#ifndef _MMAP_H_
#define _MMAP_H_

#include <u.h>
#include <libc.h>

// Define necessary types and constants if needed

void* sys_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
int   sys_mprotect(void *addr, size_t len, int prot);
int   sys_munmap(void *addr, size_t length);

#endif // _MMAP_H_
