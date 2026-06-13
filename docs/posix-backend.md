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
| time       | `time.h`            | time, gettimeofday, select               | done     |
| thread     | `thread.h`          | pthread_create, mutex, cond              | planned  |
| mmap       | `mmap.h`            | mmap, mprotect, munmap                   | planned  |
| net        | `net.h`             | socket, connect, bind                    | planned  |

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

### Phase 4 — Virtual memory

- `sys_mmap`, `sys_munmap`, `sys_mprotect`
- Needed for Odin heap allocator and `core:mem/virtual`

### Phase 5 — Networking

- Socket create/bind/connect/send/recv wrappers
- Plan 9 delta: `/net` dial/listen instead of BSD sockets

### Phase 6 — Compiler integration

- Populate `sys/src/cmd/odin_runtime/common.h` as the umbrella include
- Link `libodin_posix.a` when targeting a hypothetical `odin build -target=posix-c89`
- Provide `main` / `_start` glue if compiling without Odin `base:runtime` entry

### Phase 7 — Plan 9 port

- Copy `core/os/posix/src` → `core/os/plan9/src`
- Replace POSIX includes per module (document in `docs/plan9-delta.md`)
- Build with `mk` and `8c`/`8l`

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
