# POSIX C89 Backend for Odin

This directory contains the POSIX C89 implementation layer for Odin's Plan 9 port.

## Overview

The POSIX C89 backend serves as an intermediate step between Odin's native compiler and Plan 9's native C environment. This approach allows us to:

1. Develop and test the core translation layer on standard POSIX systems
2. Ensure C89 compatibility before targeting Plan 9's c89 compiler
3. Create a well-understood baseline for Plan 9 adaptation

## Build

```bash
cd core/os/posix
make clean && make          # build all tests
make lib                    # build libodin_posix.a
./test_mem && ./test_filesys && ./test_path && ./test_dir && ./test_env && ./test_process && ./test_time
```

See [docs/posix-backend.md](../../docs/posix-backend.md) for architecture and roadmap.

## Structure

- `include/` - Header files with C89-compatible declarations
- `src/` - Source implementations using POSIX system calls
- `tests/` - Test harness for POSIX functionality

## C89 Compliance Requirements

All code in this directory must adhere to strict C89 standards:
- All variable declarations at the top of blocks
- No mixed declarations and code
- No variadic macros
- Use of standard C89 library functions only

## Target: Plan 9 c89

After the POSIX implementation is complete and tested, we will adapt this code for Plan 9's native c89 compiler.