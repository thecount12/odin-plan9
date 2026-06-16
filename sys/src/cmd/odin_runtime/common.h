/*
 * Umbrella header for the Odin C89 POSIX runtime.
 *
 * Build with:
 *   -Icore/os/posix/src/include
 *   -Isys/src/cmd/odin_runtime
 *
 * Link with libodin_posix.a (-pthread) and entry.o unless the program
 * defines its own main().
 */
#ifndef ODIN_RUNTIME_COMMON_H
#define ODIN_RUNTIME_COMMON_H

#include "sysdeps.h"
#include "mem.h"
#include "filesys.h"
#include "path.h"
#include "dir.h"
#include "env.h"
#include "process.h"
#include "sys_time.h"
#include "thread.h"
#include "mmap.h"
#include "net.h"

#endif
