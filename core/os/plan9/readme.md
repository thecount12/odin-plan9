# Plan 9 C backend for Odin

Native 9front/Plan 9 C runtime mirroring `core/os/posix/`. All sources and headers live in `src/`; build with `mk` from this directory.

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
