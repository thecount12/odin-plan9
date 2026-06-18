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
| thread     | `thread.h`          | pthread_create, mutex, cond              | done     |
| mmap       | `mmap.h`            | mmap, mprotect, munmap                   | done     |
| net        | `net.h`             | socket, bind, connect, send, recv         | done     |

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
make integration            # link hello (entry.c + odin_main + lib)
./hello
./test_mem && ./test_filesys && ./test_dir && ./test_env && ./test_process && ./test_time && ./test_thread && ./test_mmap && ./test_net
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

### Phase 7 — Plan 9 core (complete on 9front)

- `core/os/plan9/`: sysdeps, mem, filesys, process, time
- `src/common.h` for `<u.h>` / `<libc.h>`; no nested header includes
- `libodin_plan9.a`, `test_filesys`, `hello` + `entry.c` glue
- Verified on `$objtype` **6** (amd64) and **7** (arm64)

### Phase 8 — Plan 9 paths & environment

Port POSIX Phase 2 modules to Plan 9 C (reference: `core/os/posix/src/`):

| Module | POSIX source | Plan 9 APIs (see [plan9-delta.md](plan9-delta.md)) |
|--------|--------------|-----------------------------------------------------|
| path   | `path.c`     | `getwd`, `chdir` |
| dir    | `dir.c`      | `open` + `dirread` on directory fd |
| env    | `env.c`      | `getenv`; `putenv` / `setenv` as available |

Deliverables:

- `path.h` / `path.c`, `dir.h` / `dir.c`, `env.h` / `env.c` under `core/os/plan9/src/`
- `test_path`, `test_dir`, `test_env` binaries in `mkfile`
- Append modules to `libodin_plan9.a`

Status: **complete** on 9front (objtypes 6 and 7).

### Phase 9 — POSIX threading (complete)

- `sys_mutex_create` / `lock` / `unlock` / `destroy`
- `sys_cond_create` / `wait` / `signal` / `broadcast` / `destroy`
- `sys_thread_create` / `sys_thread_join`
- Build with `-pthread` in `CFLAGS` and `LDFLAGS`
- Plan 9 delta (Phase 13): `procrfork`, `Lock`, channels — see [plan9-delta.md](plan9-delta.md)

### Phase 10 — Virtual memory (POSIX, complete)

- `sys_mmap`, `sys_munmap`, `sys_mprotect`
- `ODIN_PROT_*` and `ODIN_MAP_*` flag enums (mapped to `PROT_*` / `MAP_*` in `mmap.c`)
- Plan 9 delta (Phase 13): segment attach / `/dev/swap`

### Phase 11 — Networking (POSIX, complete)

- `sys_socket`, `sys_bind`, `sys_listen`, `sys_accept`, `sys_connect`, `sys_send`, `sys_recv`
- `SysSockAddr` + `sys_sockaddr_in()` helper (no `<netinet/in.h>` in public headers)
- Plan 9 delta (Phase 13): `dial`, `announce`, `/net/cs`

### Phase 12 — Compiler integration (POSIX, complete)

POSIX end-to-end glue (mirrors Plan 9 `hello`):

| Piece | Path |
|-------|------|
| Entry `main` | `sys/src/cmd/odin_runtime/posix/entry.c` |
| Example `odin_main` | `core/os/posix/examples/hello/hello.c` |
| Static library | `libodin_posix.a` |

```bash
cd core/os/posix
make integration
./hello    # hello from odin posix-c89
```

Odin compiler wiring (`odin build -target=posix-c89`) is future work; this proves the link model.

### Phase 14 — Plan 9 compiler integration (complete)

Same link model on 9front:

| Piece | Path |
|-------|------|
| Entry `main` | `sys/src/cmd/odin_runtime/plan9/entry.c` |
| Example `odin_main` | `core/os/plan9/examples/hello/hello.c` |
| Static library | `libodin_plan9.a` |
| Link driver | `core/os/plan9/link.rc` → `odin-plan9-link` after `mk install` |

```sh
cd core/os/plan9
mk integration
./hello    # hello from odin plan9

# link compiler-generated objects (when available):
./link.rc -o myprog generated.$O
```

LLVM `odin build -target:plan9_*` is future work; objects must be Plan 9 `$O` format from C codegen (`8c`), not LLVM `.o`.

### Phase 13 — Plan 9 advanced modules (complete)

| Module | Plan 9 implementation |
|--------|----------------------|
| thread | `Lock`/`Rendez` + `libthread` (`proccreate`, channels) in `sys_thread.c` |
| mmap | `segattach` / `segprotect` / `segdetach` |
| net | `announce` / `listen` / `accept` / `dial` + `read`/`write` |

```sh
cd core/os/plan9
mk
./test_mmap
./test_thread   # links libthread.a
./test_net      # links libthread.a
```

Header note: `sys_thread.h` (not `thread.h`) avoids clashing with libc `thread.h`.

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
