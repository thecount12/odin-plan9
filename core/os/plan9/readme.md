# Plan 9 C backend for Odin

Native 9front/Plan 9 C runtime mirroring `core/os/posix/`. All sources and headers live in `src/`; build with `mk` from this directory.

`src/common.h` is the only place for `<u.h>` and `<libc.h>`. Other headers declare APIs only; each `.c` file includes what it needs.

```sh
cd core/os/plan9
mk
./test_filesys
./hello
```

See [docs/plan9-delta.md](../../docs/plan9-delta.md) for API replacements.

## Status

Phase 7 (in progress): sysdeps, mem, filesys, process, time, test_filesys, hello.

Not yet ported: path, dir, env, thread, mmap, net.
