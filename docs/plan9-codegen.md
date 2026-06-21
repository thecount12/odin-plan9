# Plan 9 C codegen

Odin on Plan 9 uses **compiler emission → C89 → `$objtype`c → `link.rc`**, not LLVM on the target.

## Pipeline

```
hello.odin
    │
    ▼  ./odin build … -backend:plan9-c -target:plan9_arm64 -out:hello.c   (Mac)
hello.c  (exports odin_main, not main)
    │
    ▼  cd core/os/plan9 && ./build.rc -o hello hello.c   (9front)
hello.$O + entry.$O + libodin_plan9.a
    │
    ▼  link.rc
./hello
```

Runtime glue:

| Piece | Role |
|-------|------|
| `entry.c` | Plan 9 `main()` → `odin_main()` → `exits(nil)` |
| `libodin_plan9.a` | `sys_*` implementations |
| `link.rc` | `$objtypel prog.$O entry.$O libodin_plan9.a` |
| `build.rc` | Compile `.c` → `$O`, then invoke `link.rc` |
| `include/odin_generated.h` | Umbrella header for generated programs |

## Milestones

| Milestone | Scope | Status |
|-----------|-------|--------|
| M0 | Manual `examples/hello/hello.c` + `mk integration` | done |
| M1 | `build.rc` (`.c` → `$O` → link) | done |
| M2 | `cgen` bootstrap tool | done (superseded by compiler) |
| M3–M4 | Bootstrap C emitter | superseded by `-backend:plan9-c` |
| M5 | Compiler backend: hello, foreign calls, strings | done |
| M5d | Structs, `switch`, helpers (`hello_adv`) | done |
| M6 | `import "core:fmt"` via `fmt_plan9.odin` (string `println`) | done |
| M7 | `..any` variadic calls, range-for, multi-arg `println` | done |
| M8 | Integer `println`, native C arrays, C89 fixes | **done, verified on gabriel (arm64)** |

## Verified example: `fmt_hello`

**Source:** `core/os/plan9/examples/fmt_hello/fmt_hello.odin`

```odin
import "core:fmt"

main :: proc() {
    fmt.println("hello from odin plan9 fmt")
    fmt.println("hello", "world")
    fmt.println("answer", 42)
}
```

**Expected output:**

```
hello from odin plan9 fmt
hello world
answer 42
```

### Workflow (Mac → 9front)

Develop the compiler on Mac; sync via git; copy or regenerate C on the machine.

**On Mac** (after `./build_odin.sh`):

```sh
./odin build core/os/plan9/examples/fmt_hello/fmt_hello.odin \
    -file -backend:plan9-c -target:plan9_arm64 \
    -out:core/os/plan9/examples/fmt_hello/fmt_hello.c
```

Use `-target:plan9_amd64` on amd64 9front (e.g. uriel).

**On 9front** (must run from `core/os/plan9` — `build.rc` needs `-Iinclude -Isrc`):

```sh
cd core/os/plan9
rm -f fmt_hello.7          # stale $O after regen
./build.rc -o fmt_hello examples/fmt_hello/fmt_hello.c
./fmt_hello
```

The `.odin` file lives in the repo; the `.c` file is the build artifact (regenerated on Mac or copied over).

### Other examples

```sh
# Mac codegen
./odin build core/os/plan9/examples/hello/hello.odin -file -backend:plan9-c -target:plan9_arm64 -out:hello.c
./odin build core/os/plan9/examples/hello_adv/hello_adv.odin -file -backend:plan9-c -target:plan9_arm64 -out:hello_adv.c

# 9front
cd core/os/plan9
./build.rc -o hello examples/hello/hello.c
./build.rc -o hello_adv examples/hello_adv/hello_adv.c
```

## `fmt` on Plan 9 (bootstrap)

Full `core:fmt` (variadic formatting, `%v`, reflect, `core:io`) is not emitted yet. Plan 9 uses a bootstrap implementation:

| File | Role |
|------|------|
| `core/fmt/fmt_plan9.odin` | `#+build plan9` — `println`/`print` via `..any` + `sys_write` |
| `core/fmt/fmt_os.odin` | Excluded on Plan 9 (`#+build !plan9`) |

`fmt_plan9.odin` today:

- Accepts variadic `..any` arguments
- Prints **strings** and **ints** (via `typeid_of` checks)
- Space-separates args; appends newline for `println`

## C backend capabilities (current)

**Types:** basics, pointers, multi-pointers, strings, `any`, `typeid`, structs, enums, slices, native fixed arrays, `bit_set` (as backing integer), unions, maps, dynamic arrays, SIMD vectors (as array-like structs).

**Statements:** `if`/`else`, C `for`, range-for over slices (`for _, i in args`), `switch`, assignments, returns.

**Expressions:** literals, calls, casts (`u8(x)` → `(unsigned char)(x)`), selectors, indexing, slice expressions, `typeid_of`, builtins `len` / `raw_data`, variadic call packing into stack `[]any`.

**Packages:** emits min-dependency procs from all packages except `builtin`, `intrinsics`, and `runtime` (unless runtime flag set).

**Not emitted yet:** full `core:fmt`/`reflect`, interfaces, procedure types in arbitrary contexts, most of `base/runtime`, maps/union logic at runtime, general `%` formatting.

## Generated C rules (Plan 9 / C89)

- Export **`odin_main(int argc, char **argv)`** — `entry.c` owns `main`.
- User `main :: proc()` → **`odin_user_main`** (static), called from `odin_main`.
- Include **`odin_generated.h`** only; do not redeclare `sys_*`.
- **C89:** declarations at block start; `/* */` in headers; forward declarations for all functions before use.
- **Fixed arrays** emit as native C arrays (`unsigned char buf[32]`), not wrapped structs.
- **`&array`** for `^[N]T` params decays to the array name when lowered to `unsigned char *` (Plan 9 rejects `&buf` as `unsigned char (*)[32]` vs `unsigned char *`).
- **Slice / composite types** used in expressions are declared at file scope before any function bodies (never inline `typedef` inside a function).
- Prefer **`sys_write`**, **`sys_strlen`**, etc. — not Plan 9 `print` in generated code.

Implementation: `src/c_backend.cpp` (`-backend:plan9-c`, `TargetOs_plan9`).

## Next steps

Ordered by dependency. Each should stay testable on gabriel via `build.rc`.

### M9 — Richer bootstrap `fmt`

- Extend `_print_any` in `fmt_plan9.odin`: `bool`, `i64`, `cstring`, runes
- No full reflect; keep `typeid_of` dispatch
- Example: `fmt.println("x", true, 123)`

### M10 — Runtime emission (foundation)

- `-define:ODIN_PLAN9_EMIT_RUNTIME=true` emits `base/runtime` + `__$startup_runtime`
- Codegen blockers already hit in probing: **procedure types**, then iterate on whatever follows
- Goal: allocator + `Type_Info` far enough for real stdlib, not full desktop parity yet

### M11 — `core:os` from generated code

- Small example: open/read/write or `core:os` stdout path using `libodin_plan9.a`
- Depends on M10 or narrow foreign-only subset
- Validates Odin `core:os` bindings beyond typecheck

### M12 — Full `core:fmt` (long term)

- Variadic formatting with `%`, `core:io`, interfaces, reflect
- Likely requires M10 + large codegen surface (interfaces, dynamic arrays, type info tables)
- Replace or subsume `fmt_plan9.odin` bootstrap

### Parallel / housekeeping

- Register `ODIN_PLAN9_EMIT_RUNTIME` in project defineables (silences unused-define warning)
- Checked-in generated `.c` for CI smoke tests optional; source of truth remains `.odin` + Mac codegen
- amd64 verification on uriel (same commands, `-target:plan9_amd64`)

See also [plan9-runtime.md](plan9-runtime.md), [plan9-delta.md](plan9-delta.md), [README-9front.md](../README-9front.md).
