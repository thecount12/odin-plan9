# Plan 9 C backend for Odin

Native 9front/Plan 9 C runtime mirroring `core/os/posix/`. Same `sys_*` API; different libc calls underneath.

## Build (on 9front)

Copy or bind this tree into your Plan 9 environment, then:

```sh
cd core/os/plan9
mk          # builds libodin_plan9.a, test_filesys, hello
./test_filesys
./hello
```

Requires `$objtype` set (e.g. `8` for amd64 on 9front).

## Layout

- `src/include/` — portable headers (same names as posix)
- `src/*.c` — Plan 9 implementations (`#include <u.h>` / `<libc.h>`)
- `examples/hello/` — integration smoke test
- `mkfile` — `8c` / `8l` build rules

## Runtime entry

- `sys/src/cmd/odin_runtime/plan9/common.h` — umbrella include
- `sys/src/cmd/odin_runtime/plan9/entry.c` — `main` → `odin_main` → `exits`

Generated Odin C defines `odin_main()`; do not define `main()`.

See [docs/plan9-delta.md](../../docs/plan9-delta.md) for API replacements.

## Status

Phase 7 (in progress): sysdeps, mem, filesys, process, time, hello link test.

Not yet ported: path, dir, env, thread, mmap, net.
