# Plan 9 C backend for Odin

Native 9front/Plan 9 C runtime mirroring `core/os/posix/`. Same `sys_*` API; different libc calls underneath.

## Build (on 9front)

```sh
cd core/os/plan9
mk
./test_filesys
./hello
```

Requires `$objtype` set (e.g. `8` for amd64 on 9front).

See [docs/plan9-delta.md](../../docs/plan9-delta.md) for API replacements.

## Status

Phase 7 (in progress): sysdeps, mem, filesys, process, time, test_filesys, hello.

Not yet ported: path, dir, env, thread, mmap, net.
