# Odin port — project rules

## Goal
Portable **POSIX C** first, then native **Plan 9 C** (9front amd64, `8c`/`8l`).

## Languages
- **Work:** Python (reference only)
- **This repo:** POSIX C → Plan 9 C; later Go/Lux as needed

## POSIX phase
- C89, build with gcc/clang + Makefile
- No Plan 9-only APIs (`u.h`, `exits`, `#` paths, etc.)

## Plan 9 phase
- `#include <u.h>` and `#include <libc.h>`
- `exits(nil)` / `exits("msg")`, `print`, `fprint(2, ...)`
- No pthread, no BSD errno/socket style unless wrapped
- Build: `8c`, `8l`, `mk`

## Porting workflow
1. Implement and test under `posix/` (see [posix-backend.md](posix-backend.md))
2. Document deltas in [plan9-delta.md](plan9-delta.md)
3. Port to `core/os/plan9/` with minimal logic changes

When porting, always list API replacements (malloc, open/read/write, paths, errors).

## Style
- Small files, explicit structs, minimal libc surface
- No drive-by refactors