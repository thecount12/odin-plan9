# POSIX C89 backend build rules

CFLAGS = -std=c89 -pedantic -Wall -I$(SRCDIR) -pthread
LDFLAGS = -pthread
AR = ar
ARFLAGS = rcs

SRCDIR = src
INCDIR = $(SRCDIR)/include
TESTDIR = $(SRCDIR)/tests
LIB = libodin_posix.a
RUNTIME = ../../../sys/src/cmd/odin_runtime/posix
EXAMPLE = examples/hello
ENTRY_O = $(RUNTIME)/entry.o

HDRS = $(INCDIR)/sysdeps.h \
       $(INCDIR)/filesys.h \
       $(INCDIR)/path.h \
       $(INCDIR)/dir.h \
       $(INCDIR)/env.h \
       $(INCDIR)/process.h \
       $(INCDIR)/mem.h \
       $(INCDIR)/time.h \
       $(INCDIR)/thread.h \
       $(INCDIR)/mmap.h \
       $(INCDIR)/net.h

SRCS = $(SRCDIR)/sysdeps.c \
       $(SRCDIR)/filesys.c \
       $(SRCDIR)/path.c \
       $(SRCDIR)/dir.c \
       $(SRCDIR)/env.c \
       $(SRCDIR)/process.c \
       $(SRCDIR)/mem.c \
       $(SRCDIR)/time.c \
       $(SRCDIR)/thread.c \
       $(SRCDIR)/mmap.c \
       $(SRCDIR)/net.c

OBJS = $(SRCS:.c=.o)

TEST_SRCS = $(TESTDIR)/test_mem.c \
            $(TESTDIR)/test_process.c \
            $(TESTDIR)/test_time.c \
            $(TESTDIR)/test_filesys.c \
            $(TESTDIR)/test_path.c \
            $(TESTDIR)/test_dir.c \
            $(TESTDIR)/test_env.c \
            $(TESTDIR)/test_thread.c \
            $(TESTDIR)/test_mmap.c \
            $(TESTDIR)/test_net.c

TEST_OBJS = $(TEST_SRCS:.c=.o)
TEST_BINS = test_mem test_process test_time test_filesys test_path test_dir test_env test_thread test_mmap test_net

all: test

lib: $(LIB)

$(LIB): $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)

$(SRCDIR)/%.o: $(SRCDIR)/%.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

$(TESTDIR)/%.o: $(TESTDIR)/%.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

test: $(OBJS) $(TEST_BINS)

test_mem: $(TESTDIR)/test_mem.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

test_process: $(TESTDIR)/test_process.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

test_time: $(TESTDIR)/test_time.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

test_filesys: $(TESTDIR)/test_filesys.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

test_path: $(TESTDIR)/test_path.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

test_dir: $(TESTDIR)/test_dir.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

test_env: $(TESTDIR)/test_env.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

test_thread: $(TESTDIR)/test_thread.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

test_mmap: $(TESTDIR)/test_mmap.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

test_net: $(TESTDIR)/test_net.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

integration: lib hello

hello: $(EXAMPLE)/hello.o $(ENTRY_O) $(LIB)
	$(CC) $(LDFLAGS) -o $@ $(EXAMPLE)/hello.o $(ENTRY_O) $(LIB)

$(EXAMPLE)/hello.o: $(EXAMPLE)/hello.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

$(ENTRY_O): $(RUNTIME)/entry.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(SRCDIR)/*.o $(TESTDIR)/*.o
	rm -f $(TEST_BINS) $(LIB) hello $(ENTRY_O) $(EXAMPLE)/hello.o

.PHONY: all clean test lib integration
