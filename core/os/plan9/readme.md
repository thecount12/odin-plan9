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
mk integration
./link.rc -o myprog hello.7
```

Or compile from C with the build driver:

```sh
mk cgen
./cgen/cgen hello > /tmp/hello.c
./build.rc -o hello /tmp/hello.c
```

| Piece | Path |
|-------|------|
| Entry `main` | `sys/src/cmd/odin_runtime/plan9/entry.c` |
| Example `odin_main` | `core/os/plan9/examples/hello/hello.c` |
| Odin spec | `core/os/plan9/examples/hello/hello.odin` |
| Static library | `libodin_plan9.a` |
| Link driver | `link.rc` |
| Build driver | `build.rc` |
| C codegen | `cgen/cgen` |

See [docs/plan9-codegen.md](../../docs/plan9-codegen.md).

## Status

| Phase | Scope | Status |
|-------|-------|--------|
| 7–8 | core I/O, path, dir, env | done |
| 13 | mmap, sys_thread, net | done |
| 14 | integration glue (`entry.c`, `link.rc`, `examples/hello`) | done |
| 15 | `cgen` + `build.rc` | started |
