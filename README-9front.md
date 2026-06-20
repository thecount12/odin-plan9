# Odin on Plan 9 / 9front

Porting Odin via a **C89 runtime backend** (`core/os/plan9/`), not LLVM-on-Plan9 (yet).

## Status

| Layer | Status |
|-------|--------|
| `libodin_plan9.a` (sysdeps … net) | done, tested on amd64 + arm64 |
| Integration glue (`entry.c`, `hello`, `link.rc`) | done |
| C codegen (`cgen`, `build.rc`) | done on 9front |
| Odin compiler `-backend:plan9-c` | 5a–5d done; **fmt_hello** (`core:fmt` string println) |
| `base/runtime` Plan 9 (`ODIN_OS=.Plan9`) | started (5d) |
| `core:os` Plan 9 bindings | typecheck ok |

See [docs/plan9-runtime.md](docs/plan9-runtime.md).

## Quick start (9front)

```sh
cd core/os/plan9
mk
mk integration
./hello
```

## Linking generated code

When the compiler emits C with `odin_main()`:

```sh
cd core/os/plan9
mk cgen
./cgen/cgen hello > hello.c
./build.rc -o hello hello.c
./hello
```

Or from checked-in C:

```sh
./build.rc -o hello examples/hello/hello.c
```

See [docs/plan9-codegen.md](docs/plan9-codegen.md).

See [core/os/plan9/readme.md](core/os/plan9/readme.md) and [docs/plan9-delta.md](docs/plan9-delta.md).

## Architecture

```
Odin compiler (future C emit) ──► generated .c with odin_main()
                                        │
                                        ▼
                              8c ──► myprog.$O
                                        │
          entry.c (main→odin_main) ─────┼──► 8l ──► executable
          libodin_plan9.a (sys_*) ──────┘
```

POSIX reference implementation: `core/os/posix/` + [docs/posix-backend.md](docs/posix-backend.md).
