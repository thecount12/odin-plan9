# Odin Port to Plan 9

This document outlines the approach for porting Odin to Plan 9 systems.

## Overview

Odin is a programming language that currently targets POSIX systems. This porting effort involves:
1. First implementing a complete POSIX C backend
2. Then adapting that to work with Plan 9's C compiler and system calls

## Current Status

- Working on POSIX C implementation
- Have multiple Plan 9 bare metal installations (AMD, Intel, QEMU with ARM)
- Core directories: `core/os/9front` and `core/sys/9front`

## Implementation Strategy

### Phase 1: POSIX C Backend
Focus on creating a complete POSIX C backend that:
- Generates valid POSIX C code
- Handles all Odin language features
- Compiles correctly with standard POSIX C compilers

### Phase 2: Plan 9 Adaptation
Adapt the POSIX C output to work with Plan 9's:
- Different system calls and library functions
- File system semantics
- Threading model differences
- C compiler (c89) specific requirements

## Key Differences to Consider

### System Calls
Plan 9 uses a different set of system calls compared to POSIX:
- File operations are handled through the 9P protocol
- Process management differs from fork/exec models
- Memory management has different semantics

### Library Functions
Many standard C library functions have different implementations or are unavailable in Plan 9.

### File System
Plan 9 uses a unified namespace with 9P protocol for file access.

## Getting Started

1. Review existing POSIX implementation in the codebase
2. Identify system call differences between POSIX and Plan 9
3. Create a test harness that can compile and run Odin programs on both systems
4. Gradually add Plan 9 specific functionality while maintaining POSIX compatibility

## Testing Approach

Use your existing installations:
- AMD architecture
- Intel architecture
- QEMU ARM emulation
- Test each architecture with the same Odin codebase

This approach allows for cross-platform testing and validation of the porting efforts.