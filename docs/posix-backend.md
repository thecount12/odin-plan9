# C89 POSIX backend for Odin

Portable C89 runtime layer under `core/os/posix/`. This is the first phase of the Plan 9 port described in [project.md](project.md): prove the syscall surface on POSIX, then translate to Plan 9 C.

## Architecture

```
Odin compiler (LLVM) ──► Odin-generated C
                              │
                              ▼
                    libodin_posix.a  (core/os/posix)
                              │
                              ▼
                    POSIX libc / kernel
```

The backend is **not** a new Odin compiler. It is a thin C89 `sys_*` layer that:

1. Wraps POSIX syscalls with Odin/Plan 9 semantics (flag values, error model, struct layouts).
2. Compiles with `gcc`/`clang -std=c89 -pedantic`.
3. Serves as the reference implementation before porting each module to Plan 9 `8c`/`8l`.

### Module layout

| Module     | Header              | POSIX APIs used                          | Status   |
|------------|---------------------|------------------------------------------|----------|
| sysdeps    | `sysdeps.h`         | errno, types                             | done     |
| mem        | `mem.h`             | malloc, free, memcpy                     | done     |
| filesys    | `filesys.h`         | open, read, write, stat, mkdir, unlink   | done     |
| path       | `path.h`            | getcwd, chdir                            | done     |
| dir        | `dir.h`             | opendir, readdir, closedir               | done     |
| env        | `env.h`             | getenv, setenv, unsetenv                 | done     |
| process    | `process.h`         | fork, exec, wait, getpid                 | done     |
| time       | `sys_time.h`        | time, gettimeofday, select               | done     |
| thread     | `thread.h`          | pthread_create, mutex, cond              | done     |
| mmap       | `mmap.h`            | mmap, mprotect, munmap                   | done     |
| net        | `net.h`             | socket, bind, connect, send, recv       | done     |

### Naming conventions

- All exported functions: `sys_<verb>` (e.g. `sys_open`, `sys_getenv`).
- Odin open flags: `ODIN_O_*` enum values; public aliases `O_*` for callers that do not include `<fcntl.h>`.
- Implementation files include POSIX headers; callers include only `include/*.h`.
- Errors: thread-local `errno_` via `sys_err()` / `sys_seterr()`. POSIX `errno` is copied in at the syscall boundary.

### C89 rules (enforced)

- Declarations at the top of every block.
- No `//` comments in headers if targeting Plan 9 `8c` (use `/* */` in public headers).
- No `long long` literals; cast via `(ulonglong)`.
- `stdint.h` types (`uint32_t`, `uint64_t`) for fixed-width fields.

## Build

```bash
cd core/os/posix
make clean && make          # build tests
make lib                    # build libodin_posix.a
./test_mem && ./test_filesys && ./test_dir && ./test_env && ./test_process && ./test_time
```

## Phased roadmap

### Phase 1 — Core I/O (complete)

- Memory, files, processes, time
- Odin flag mapping for `open`
- Test harness per module

### Phase 2 — Paths & environment (complete)

- `sys_getcwd`, `sys_chdir`
- `sys_opendir` / `sys_readdir` / `sys_closedir`
- `sys_getenv`, `sys_setenv`, `sys_unsetenv`

### Phase 3 — Threading

- `sys_thread_create`, `sys_mutex_*`, `sys_cond_*`
- Map to `pthread` with `-pthread` in LDFLAGS
- Document Plan 9 delta: no pthread on 9front; use `procrfork` / channels instead

### Phase 4 — Virtual memory (complete)

- `sys_mmap`, `sys_munmap`, `sys_mprotect`
- Odin `ODIN_MAP_*` / `ODIN_PROT_*` flag mapping (Darwin `MAP_ANON`, Linux `MAP_ANONYMOUS`)
- Anonymous and file-backed mmap tests

## Plan 9 API replacements (reference)

| POSIX              | Plan 9 / 9front        |
|--------------------|------------------------|
| `open/read/write`  | `open/read/write` (different flag bits) |
| `malloc/free`      | `malloc/free` (often same) |
| `fork/exec/wait`   | `rfork/exec/wait`      |
| `opendir/readdir`  | same names, `Dir` struct differs |
| `getenv`           | `getenv`               |
| `pthread_*`        | `procrfork`, `lock`, channels |
| `socket`           | `dial`, `announce`, `/net` |
| `mmap`             | `/dev/swap`, segment attach |


- `sys_mmap`, `sys_munmap`, `sys_mprotect`
- Needed for Odin heap allocator and `core:mem/virtual`

### Memory Management Differences

- **POSIX**: Uses `mmap`, `mprotect`, and `munmap` for memory mapping operations.
- **Plan 9**: Uses different mechanisms for memory management:
  - `/dev/swap` for mapping
  - Segment attachment for protection changes
  - Detach segments to unmap memory

### Threading Differences

- **POSIX**: Uses pthreads for threading.
- **Plan 9**: Uses `procrfork()` and channels (`Chan`) for concurrency.

### Error Handling Differences

- **POSIX**: Uses `errno` for error reporting.
- **Plan 9**: Uses `print`, `fprint(2, ...)`, and other Plan 9-specific error handling methods.


### Phase 5 — Networking (complete)

- `sys_socket`, `sys_socketpair`, `sys_bind`, `sys_connect`, `sys_listen`, `sys_accept`
- `sys_send`, `sys_recv`, `sys_shutdown`
- `SysSockAddrUn` / `SysSockAddrIn` with Odin address-family mapping
- Plan 9 delta: `/net` dial/listen instead of BSD sockets

### Phase 6 — Compiler integration (complete)

- `sys/src/cmd/odin_runtime/common.h` — umbrella include for generated C
- `sys/src/cmd/odin_runtime/entry.c` — `main()` → `odin_main()` glue
- `sys/src/cmd/odin_runtime/odin_runtime.h` — contract for generated programs
- `examples/hello/hello.c` — integration smoke test
- `make integration` — builds `libodin_posix.a`, links `hello`, verifies end-to-end link

#### Linking a generated program

```bash
cd core/os/posix
make integration
./hello
# hello from odin posix-c89
```

Manual link (from repo root):

```bash
cd core/os/posix && make lib
cc -std=gnu89 -pedantic -Wall \
   -Icore/os/posix/src/include \
   -Isys/src/cmd/odin_runtime \
   -c core/os/posix/examples/hello/hello.c \
   -o /tmp/hello.o
cc -std=gnu89 -pedantic -Wall \
   -Icore/os/posix/src/include \
   -Isys/src/cmd/odin_runtime \
   -c sys/src/cmd/odin_runtime/entry.c \
   -o /tmp/entry.o
cc -pthread -o hello /tmp/hello.o /tmp/entry.o core/os/posix/libodin_posix.a
```

Generated C must define `int odin_main(int argc, char **argv)` and include `"common.h"`.

### Phase 7 — Plan 9 port (in progress)

- `core/os/plan9/` — 9front C runtime (mirrors posix API)
- `docs/plan9-delta.md` — API replacement table
- `sys/src/cmd/odin_runtime/plan9/` — entry glue (`exits` not `return`)
- Build on 9front: `cd core/os/plan9 && mk`
- **Done:** sysdeps, mem, filesys, process, time, test_filesys, hello
- **Remaining:** path, dir, env, thread, mmap, net

## Plan 9 API replacements (reference)

| POSIX              | Plan 9 / 9front        |
|--------------------|------------------------|
| `open/read/write`  | `open/read/write` (different flag bits) |
| `malloc/free`      | `malloc/free` (often same) |
| `fork/exec/wait`   | `rfork/exec/wait`      |
| `opendir/readdir`  | same names, `Dir` struct differs |
| `getenv`           | `getenv`               |
| `pthread_*`        | `procrfork`, `lock`, channels |
| `socket`           | `dial`, `announce`, `/net` |
| `mmap`             | `/dev/swap`, segment attach |

## Adding a new module

1. Add `src/include/foo.h` and `src/foo.c`.
2. Append to `SRCS` and `HDRS` in `Makefile`.
3. Add `src/tests/test_foo.c` and a `test_foo` target.
4. Document POSIX → Plan 9 delta when the module is stable.
