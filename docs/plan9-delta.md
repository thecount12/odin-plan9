# POSIX → Plan 9 API delta

Reference for porting `core/os/posix/` to `core/os/plan9/`.

## Build

| POSIX | Plan 9 / 9front |
|-------|-----------------|
| `gcc -std=gnu89` | `8c` (or `$objtype` compiler) |
| `Makefile` | `mkfile` |
| `libodin_posix.a` | `libodin_plan9.a` |
| `#include <unistd.h>` | `#include <u.h>` + `#include <libc.h>` |
| `main()` returns `int` | `void main()` + `exits(nil)` |

## Types

| POSIX / typical C | Plan 9 / 9front |
|-------------------|-----------------|
| `uint64_t` / `unsigned long long` | `uvlong` (not `unsigned long`, which is 32-bit) |
| `int64_t` | `vlong` |
| `%llu` in `printf` | `%llud` in `print` for `uvlong` |

`sysdeps.h` maps `ulonglong` → `uvlong` so `Stat.length` and `Stat.ino` stay 64-bit.

## Errors

| POSIX | Plan 9 |
|-------|--------|
| `errno` | `errstr(buf, n)` after `-1` return (9front) |
| `sys_seterr_posix()` | `sys_seterr_plan9()` sets `errno_ = ERR_IO` |

## Memory

| POSIX | Plan 9 |
|-------|--------|
| `malloc` / `free` | same (`libc.h`) |
| `memcpy` / `memset` | same |

## Files

| POSIX | Plan 9 |
|-------|--------|
| `open(path, flags)` | `open(path, mode)` or `create(path, mode, perm)` |
| `O_CREAT` | `create()` when `ODIN_O_CREATE` set |
| `O_RDONLY/WRONLY/RDWR` | `OREAD` / `OWRITE` / `ORDWR` |
| `O_TRUNC` | `OTRUNC` |
| `O_EXCL` | `OEXCL` |
| `read` / `write` | same signatures (`long` count) |
| `close` | `close()` (void) |
| `lseek` | `seek(fd, offset, start)` |
| `unlink` | `remove()` |
| `mkdir` | `create(path, OREAD, DMDIR\|perm)` |
| `rmdir` | `remove()` on empty dir |
| `rename` | not in libc — stub for now; use `wstat` later |
| `stat` + `struct stat` | `dirstat()` → `Dir`; map `DMDIR` not `S_IFMT` |

## Process

| POSIX | Plan 9 |
|-------|--------|
| `fork()` | `rfork(RFPROC\|RFMEM\|RFFDG)` |
| `execv` | `exec(path, argv)` |
| `_exit` | `exits(msg)` |
| `wait(&status)` | `wait()` → `Waitmsg*` |
| `getpid` | `getpid()` |
| `getppid` | stub / platform-specific |

## Time

| POSIX | Plan 9 |
|-------|--------|
| `time(NULL)` | `time(0)` |
| `select` sleep | `osmillisecond()` busy-wait loop |
| `gettimeofday` | coarse: `time(0)` (no sub-second yet) |

## Threading (not yet ported)

| POSIX | Plan 9 |
|-------|--------|
| `pthread_create` | `procrfork` / `libthread` / channels |
| `pthread_mutex_*` | `Lock` / channel sync |

## mmap (not yet ported)

| POSIX | Plan 9 |
|-------|--------|
| `mmap` | `/dev/swap`, segment attach |
| `mprotect` | segment protection APIs |
| `munmap` | segment detach |

## Networking (not yet ported)

| POSIX | Plan 9 |
|-------|--------|
| `socket` / `connect` | `dial`, `announce`, `/net/cs` |
| `bind` / `listen` | `announce` |

## Module port status

| Module | Plan 9 status |
|--------|---------------|
| sysdeps | done |
| mem | done |
| filesys | done |
| process | done (basic) |
| sys_time | done (coarse) |
| path | planned |
| dir | planned |
| env | planned |
| thread | planned |
| mmap | planned |
| net | planned |

## Workflow

1. Implement and test module under `posix/` on Mac.
2. Port the same logic to `plan9/` using this table.
3. Build on 9front: `cd core/os/plan9 && mk`.
4. Run tests on 9front: `./test_filesys`, `./hello`.
