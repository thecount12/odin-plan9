/*
 * Example generated program for POSIX-C89 integration testing.
 * Simulates output from: odin build -target:posix-c89
 */
#include "common.h"
#include "odin_runtime.h"

int odin_main(int argc, char **argv) {
    const char *msg;
    long len;

    (void)argc;
    (void)argv;

    msg = "hello from odin posix-c89\n";
    len = sys_strlen(msg);
    sys_write(1, msg, len);
    return 0;
}
