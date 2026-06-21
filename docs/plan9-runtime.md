# Plan 9 runtime (Phase 5d)

Wire `base/runtime` and stdlib (`core:fmt`, allocators, etc.) to `libodin_plan9.a`.

## Status

| Piece | Status |
|-------|--------|
| `TargetOs_plan9` + `-target:plan9_amd64/arm64` | done |
| `os_specific_plan9.odin` (`_stderr_write`, `_exit`) | done |
| `heap_allocator_plan9.odin` (`sys_malloc` / `sys_free`) | done |
| `core:os` + `core:sync` Plan 9 bindings | done (typecheck) |
| **`core:fmt` bootstrap** (`fmt_plan9.odin`) | **done — `fmt_hello` verified on gabriel** |
| Emit `base/runtime` from `-backend:plan9-c` | blocked (proc types, type surface) |
| `entry.c` runtime startup (`__$startup_runtime`) | C glue emitted when flag set; not linked end-to-end |
| Full `core:fmt` / reflect / `Type_Info` | not started |

## Compiler targets

```sh
# gabriel (arm64)
./odin build hello.odin -file -backend:plan9-c -target:plan9_arm64 -out:hello.c

# uriel (amd64)
./odin build hello.odin -file -backend:plan9-c -target:plan9_amd64 -out:hello.c
```

With `-backend:plan9-c` alone, the compiler defaults to `plan9_arm64`. `ODIN_OS` is `.Plan9`.

Optional (in progress):

```sh
-define:ODIN_PLAN9_EMIT_RUNTIME=true
```

Emits `__$startup_runtime` / `__$cleanup_runtime` and includes `base/runtime` procs in codegen. Currently fails during type emission (procedure types, SIMD proc signatures, etc.) — see [plan9-codegen.md](plan9-codegen.md#m10--runtime-emission-foundation).

## Runtime layout

```
base/runtime/os_specific_plan9.odin      → sys_write, sys_exit
base/runtime/heap_allocator_plan9.odin   → sys_malloc, sys_free, …
core/os/plan9/libodin_plan9.a            → C sys_* (mem.c, filesys.c, …)
core/fmt/fmt_plan9.odin                  → bootstrap println (Plan 9 only)
sys/src/cmd/odin_runtime/plan9/entry.c   → main → odin_main → exits
```

Generated C includes `odin_generated.h` and must **not** redeclare `sys_*`. The C backend skips foreign procs whose names start with `sys_`.

## `fmt` today vs full stdlib

| | Bootstrap (`fmt_plan9.odin`) | Full `core:fmt` |
|--|-------------------------------|-----------------|
| Entry | `#+build plan9` | `fmt_os.odin` (excluded on Plan 9) |
| API | `println`/`print` `..any` | `%v`, interfaces, `core:io`, reflect |
| Types | string, int (more in M9) | all `any` types |
| Codegen | min-dep procs only | most of stdlib |
| Status | **works on 9front** | blocked on M10/M12 |

Smoke test:

```sh
# Mac
./odin build core/os/plan9/examples/fmt_hello/fmt_hello.odin \
    -file -backend:plan9-c -target:plan9_arm64 \
    -out:core/os/plan9/examples/fmt_hello/fmt_hello.c

# 9front
cd core/os/plan9 && ./build.rc -o fmt_hello examples/fmt_hello/fmt_hello.c && ./fmt_hello
```

## Plan 9 `core:os` bindings

Files: `core/os/*_plan9.odin`, `core/sync/*_plan9.odin`. These let programs **typecheck** with `import "core:os"` and `import "core:fmt"`. Generated code does not yet call most of `core:os` — that is M11.

## Next steps

1. **M9** — extend `fmt_plan9._print_any` (bool, i64, …) without full runtime
2. **M10** — procedure-type emission; `-define:ODIN_PLAN9_EMIT_RUNTIME=true` links `base/runtime`
3. **M11** — `core:os` example (read/write file or stdout via Odin API)
4. **M12** — retire bootstrap fmt when full formatter + reflect codegen exists

See [plan9-codegen.md](plan9-codegen.md) for C backend rules and milestone detail.
