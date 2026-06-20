# Plan 9 runtime (Phase 5d)

Wire `base/runtime` to `libodin_plan9.a` so programs can eventually use `core:fmt`, allocators, etc.

## Status

| Piece | Status |
|-------|--------|
| `TargetOs_plan9` + `-target:plan9_amd64/arm64` | done |
| `os_specific_plan9.odin` (`_stderr_write`, `_exit`) | done |
| `heap_allocator_plan9.odin` (`sys_malloc` / `sys_free`) | done |
| `core:os` Plan 9 bindings (`*_plan9.odin`) | done (typecheck) |
| Emit `base/runtime` from `-backend:plan9-c` | started (`-define:ODIN_PLAN9_EMIT_RUNTIME=true`) |
| `entry.c` runtime startup (`__$startup_runtime`) | started (C glue emitted) |
| `core:fmt` end-to-end | typecheck ok; full codegen blocked on Type_Info |

## Compiler targets

```sh
# gabriel (arm64) — default when -backend:plan9-c and no -target
./odin build hello.odin -file -backend:plan9-c -target:plan9_arm64 -out:hello.c

# uriel (amd64)
./odin build hello.odin -file -backend:plan9-c -target:plan9_amd64 -out:hello.c
```

With `-backend:plan9-c` alone, the compiler defaults to `plan9_arm64`.

`ODIN_OS` is `.Plan9` when using these targets.

## Runtime layout

Plan 9 uses the same split as POSIX:

```
base/runtime/os_specific_plan9.odin   → sys_write(2), sys_exit
base/runtime/heap_allocator_plan9.odin → sys_malloc, sys_free, …
core/os/plan9/libodin_plan9.a         → C implementations (mem.c, filesys.c, …)
sys/src/cmd/odin_runtime/plan9/entry.c → main → odin_main → exits
```

Runtime Odin code declares `foreign` `sys_*` procs (same names as `odin_generated.h`). The C backend must **not** re-emit `extern sys_*` for those (already handled for user `foreign` blocks).

## Next steps (5d continued)

1. **Expand codegen** — structs, `switch`/`when`, type info (blocks `core:fmt` link).
2. **Emit dependency closure** — fmt/stdlib procs beyond `init` + `runtime`.
3. **Runtime emission** — `-define:ODIN_PLAN9_EMIT_RUNTIME=true` emits `__$startup_runtime` + `base/runtime` procs.
4. **Smoke test on gabriel** — `fmt_hello.odin` → C → `build.rc`.

## Plan 9 `core:os` bindings

Files: `core/os/*_plan9.odin`, `core/sync/*_plan9.odin`. These wire `core:os` to `sys_*` in `libodin_plan9.a` so programs typecheck with `import "core:fmt"`.

Example (typecheck + stub C emit today):

```sh
./odin build core/os/plan9/examples/fmt_hello/fmt_hello.odin -file -backend:plan9-c -out:fmt_hello.c
```

## Manual hello (today)

User code still calls `sys_*` via `foreign` or helpers in the main package. Runtime emission is not required for the current `hello.odin` milestone.

See [plan9-codegen.md](plan9-codegen.md) and [plan9-delta.md](plan9-delta.md).
