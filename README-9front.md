# Odin on Plan 9 / 9front

Porting Odin via a **C89 runtime backend** (`core/os/plan9/`), not LLVM-on-Plan9 (yet).

## Status

| Layer | Status |
|-------|--------|
| `libodin_plan9.a` (sysdeps … net) | done, tested on amd64 + arm64 |
| Integration glue (`entry.c`, `hello`, `link.rc`) | done |
| Odin compiler C codegen + `odin build -target:plan9_*` | not started |

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
8c -Isrc -o myprog.$O myprog.c    # generated C
mk                               # libodin_plan9.a + entry.$O
./link.rc -o myprog myprog.$O
./myprog
```

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
