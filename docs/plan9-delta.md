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

## Headers

| POSIX | Plan 9 / 9front |
|-------|-----------------|
| many system includes | `src/common.h` only (`<u.h>` + `<libc.h>`) |
| umbrella headers | none — each `.c` includes module headers directly |

Example `.c` include order:

```c
#include "common.h"
#include "sysdeps.h"
#include "filesys.h"
```

Do not nest project `#include`s inside other header files.

## Types

| POSIX / typical C | Plan 9 / 9front |
|-------------------|-----------------|
| `uint64_t` / `unsigned long long` | `ulonglong` in `sysdeps.h` (`{ulong lo; ulong hi;}`) |
| `int64_t` | convert via `sys_ull_from_ptr()` in `.c` files |
| `%llu` in `printf` | `sys_ull_snprint()` + `print("%s", buf)` |

`ulonglong` is defined without `<u.h>`. `.c` files copy to/from `uvlong` with `sys_ull_from_ptr()` and `memmove` inside `sysdeps.c`.

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
| `select` sleep | `sleep(millisecs)` from `libc.h` |
| `gettimeofday` | coarse: `time(0)` (no sub-second yet) |

## Paths & environment (Phase 8)

| POSIX | Plan 9 |
|-------|--------|
| `getcwd` | `getwd(buf, size)` |
| `chdir` | `chdir(path)` |
| `opendir` / `readdir` | `open` + `dirreadall(fd, &buf)`; iterate cached array |
| `closedir` | single `free(buf)` — do not free individual `Dir.name` |
| `getenv` | `getenv(name)` → reads `/env/name` |
| `setenv` / `unsetenv` | `putenv(name, val)`; `remove("/env/name")` |

## Threading (Phase 13)

| POSIX | Plan 9 |
|-------|--------|
| `pthread_create` | `procrfork` / `libthread` / channels |
| `pthread_mutex_*` | `Lock` / channel sync |

## mmap (Phase 13)

| POSIX | Plan 9 |
|-------|--------|
| `mmap` | `/dev/swap`, segment attach |
| `mprotect` | segment protection APIs |
| `munmap` | segment detach |

## Networking (Phase 13)

| POSIX | Plan 9 |
|-------|--------|
| `socket` / `connect` | `dial`, `announce`, `/net/cs` |
| `bind` / `listen` | `announce` |

## Plan 9 port progress

| Phase | Scope | Status |
|-------|-------|--------|
| 7 | sysdeps, mem, filesys, process, time, hello | **done** (6 + 7) |
| 8 | path, dir, env + tests | **done** |
| 13 | thread, mmap, net | after POSIX Phases 9–11 |

See [posix-backend.md](posix-backend.md) phased roadmap.

## Workflow

1. Implement and test module under `posix/` on Mac.
2. Port the same logic to `plan9/` using this table.
3. Build on 9front: `cd core/os/plan9 && mk`.
4. Run tests on 9front: `./test_filesys`, `./hello`.
