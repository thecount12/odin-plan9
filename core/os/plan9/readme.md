# Plan 9 C backend for Odin

Native 9front/Plan 9 C runtime mirroring `core/os/posix/`. All sources and headers live in `src/`; build with `mk` from this directory.

`src/common.h` is the only place for `<u.h>` and `<libc.h>`. Other headers declare APIs only; each `.c` file includes what it needs.

```sh
cd core/os/plan9
mk
./test_filesys
./test_path
./hello
```

See [docs/plan9-delta.md](../../docs/plan9-delta.md) for API replacements.

## Status

| Phase | Scope | Status |
|-------|-------|--------|
| 7 | sysdeps, mem, filesys, process, time, test_filesys, hello | done (amd64 + arm64) |
| 8 | path, dir, env + tests | path done; dir, env next |

See [posix-backend.md](../../docs/posix-backend.md) for the full roadmap (Phases 9–13: threading, mmap, net, compiler integration).
