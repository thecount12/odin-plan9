/*
 * Program entry for Odin POSIX-C89 targets.
 * Links with libodin_posix.a; generated code supplies odin_main().
 */
#include "common.h"
#include "odin_runtime.h"

int main(int argc, char **argv) {
    int status;

    status = odin_main(argc, argv);
    return status;
}
