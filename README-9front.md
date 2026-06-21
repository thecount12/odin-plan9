# Odin on Plan 9 / 9front

Porting Odin via a **C89 runtime backend** (`core/os/plan9/`), not LLVM-on-Plan9 (yet).

## Status

| Layer | Status |
|-------|--------|
| `libodin_plan9.a` (sysdeps … net) | done, tested on amd64 + arm64 |
| Integration glue (`entry.c`, `hello`, `link.rc`) | done |
| C codegen (`build.rc`, `-backend:plan9-c`) | done on 9front |
| **`import "core:fmt"` bootstrap** (`fmt_hello`) | **done, verified on gabriel (arm64)** |
| Odin compiler `-backend:plan9-c` | M8 — variadic `..any`, ints, C89-safe emission |
| `base/runtime` Plan 9 (`ODIN_OS=.Plan9`) | bindings started; emission not linked yet |
| `core:os` Plan 9 bindings | typecheck ok; generated use not yet |

Latest verified program:

```
hello from odin plan9 fmt
hello world
answer 42
```

See [docs/plan9-codegen.md](docs/plan9-codegen.md) for the full milestone list and next steps.

## Quick start (9front)

```sh
cd core/os/plan9
mk
mk integration
./hello
```

## Compiler-generated programs

**Pipeline:** codegen on Mac (or any host with `./build_odin.sh`) → sync → `build.rc` on 9front.

```sh
# Mac
./build_odin.sh
./odin build core/os/plan9/examples/fmt_hello/fmt_hello.odin \
    -file -backend:plan9-c -target:plan9_arm64 \
    -out:core/os/plan9/examples/fmt_hello/fmt_hello.c

# 9front (from core/os/plan9)
rm -f fmt_hello.7
./build.rc -o fmt_hello examples/fmt_hello/fmt_hello.c
./fmt_hello
```

Other examples: `hello`, `hello_adv` — same pattern. Use `-target:plan9_amd64` on amd64 machines.

Or link hand-written C:

```sh
./build.rc -o hello examples/hello/hello.c
```

## Architecture

```
Odin source  ──►  odin -backend:plan9-c  ──►  program.c (odin_main)
                                                    │
                                            $objtypec -Isrc
                                                    │
              entry.c (main→odin_main) ────────────┼──► $objtypel ──► executable
              libodin_plan9.a (sys_*) ─────────────┘
```

POSIX reference: `core/os/posix/` + [docs/posix-backend.md](docs/posix-backend.md).

## Next steps (summary)

1. **M9** — bootstrap `fmt`: bool, i64, more types in `fmt_plan9.odin`
2. **M10** — emit `base/runtime` (`ODIN_PLAN9_EMIT_RUNTIME`); fix proc-type codegen
3. **M11** — generated code using `core:os` / file I/O
4. **M12** — full `core:fmt` with reflect (long term)

Details: [docs/plan9-codegen.md](docs/plan9-codegen.md#next-steps).

See also [docs/plan9-runtime.md](docs/plan9-runtime.md), [core/os/plan9/readme.md](core/os/plan9/readme.md), [docs/plan9-delta.md](docs/plan9-delta.md).
