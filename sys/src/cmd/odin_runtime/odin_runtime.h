/*
 * Odin POSIX-C89 runtime contract.
 *
 * Compiler-generated translation units should:
 *   1. #include "common.h"  (with -I pointing at core/os/posix/src/include)
 *   2. Define int odin_main(int argc, char **argv);
 *   3. Link libodin_posix.a and entry.o (or provide a custom main).
 *
 * Do not define main() in generated code unless ODIN_NO_ENTRY_POINT is set.
 */
#ifndef ODIN_RUNTIME_H
#define ODIN_RUNTIME_H

int odin_main(int argc, char **argv);

#endif
