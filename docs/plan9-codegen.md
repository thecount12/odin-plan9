# Plan 9 C codegen (Phase 15)

Odin on Plan 9 uses **C89 emission → `$objtype`c → `link.rc`**, not LLVM.

## Pipeline

```
hello.odin  ──►  cgen (future: odin build -backend:plan9-c)
                    │
                    ▼
              program.c  (exports odin_main)
                    │
              7c/6c -Isrc -o program.7
                    │
              link.rc -o program program.7
                    │
                    ▼
              ./program
```

Runtime glue (done, Phase 14):

| Piece | Role |
|-------|------|
| `entry.c` | `main()` → `odin_main()` → `exits(nil)` |
| `libodin_plan9.a` | `sys_*` POSIX-like surface |
| `link.rc` | `7l prog.7 entry.7 libodin_plan9.a` |

## Milestones

| Milestone | Scope | Status |
|-----------|-------|--------|
| M0 | Manual `examples/hello/hello.c` + `mk integration` | done |
| M1 | `build.rc` driver (`.c` → `$O` → link) | done |
| M2 | `cgen` tool: emit hello.c from template | done |
| M3 | `cgen`: expressions, calls, strings | pending |
| M4 | `cgen`: read Odin subset / IR | pending |
| M5 | Odin compiler `-backend:plan9-c` | pending |

## On 9front

```sh
cd core/os/plan9
mk integration          # lib + entry + link.rc
mk cgen
./cgen/cgen hello > /tmp/hello.c
./build.rc -o hello /tmp/hello.c
./hello
```

Or from checked-in C:

```sh
./build.rc -o hello examples/hello/hello.c
```

## Generated C rules

- Export **`odin_main(int argc, char **argv)`**, not `main`.
- Include `odin_generated.h` (pulls in `sys_*` headers).
- C89 only: `/* */` comments, declarations at block start, no `//` in headers.
- Use `sys_write`, `sys_strlen`, etc. — no raw Plan 9 `print` in generated code.

## `hello.odin`

`core/os/plan9/examples/hello/hello.odin` is the **source-of-truth** Odin program for this milestone.
`cgen hello` currently emits the equivalent C; later the compiler will emit the same shape.
