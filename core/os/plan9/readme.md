# Plan 9 C backend for Odin

Native 9front/Plan 9 C runtime mirroring `core/os/posix/`. Build with `mk` on 9front.

## Build and test

```sh
cd core/os/plan9
mk
./test_filesys
./test_path
./test_dir
./test_env
./test_mmap
./test_thread
./test_net 127.0.0.1 59999   # with listen1 running, or local self-test
```

## Compiler integration (link model)

Same contract as POSIX Phase 12:

```
entry.c:  main() → odin_main(argc, argv) → exits(nil)
generated: odin_main()  →  sys_* calls
static lib: libodin_plan9.a
```

```sh
cd core/os/plan9
mk integration          # builds lib + hello
./hello                 # hello from odin plan9
```

Link your own generated `$O` files:

```sh
mk                      # libodin_plan9.a + entry.$O
./link.rc -o myprog /path/to/generated.$O
# or after: mk install
odin-plan9-link -o myprog /path/to/generated.$O
```

| Piece | Path |
|-------|------|
| Entry `main` | `sys/src/cmd/odin_runtime/plan9/entry.c` |
| Example `odin_main` | `core/os/plan9/examples/hello/hello.c` |
| Static library | `libodin_plan9.a` |
| Link driver | `core/os/plan9/link.rc` |

`odin build -target:plan9_*` in the LLVM compiler is future work; codegen must emit C (or Plan 9 `$O`) plus `odin_main`, not `main`.

See [docs/plan9-delta.md](../../docs/plan9-delta.md) and [docs/posix-backend.md](../../docs/posix-backend.md).

## Status

| Phase | Scope | Status |
|-------|-------|--------|
| 7–8 | core I/O, path, dir, env | done |
| 13 | mmap, sys_thread, net | done |
| 14 | integration glue (`entry.c`, `link.rc`, `examples/hello`) | done |
