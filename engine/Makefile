# Makefile for the engine (CompCert-lowered core)
# Engine: CompCert 3.12 lowered core (asm/bm_core.s).
# Harness (I/O, CLI, pthreads, UTF-8): GCC.
#
# The CompCert core is the verified fragment. The harness links
# against it; CompCert output is ABI-compatible with GCC code.

CCOMP ?= $(HOME)/workspace/.tools/compcert-3.12-install/bin/ccomp
CC = gcc
CFLAGS = -Wall -Wextra -Werror -O2 -fno-builtin -pedantic-errors -std=c11 -Isrc
LDFLAGS = -static -lpthread

CORE_SRC = src/bm_core.c
CORE_ASM = asm/bm_core.s
CORE_OBJ = asm/bm_core.o
HARNESS_SRC = src/harness.c
BIN = bin/bm_compcert
TESTS = tests/test_bm_compcert
TEST_SRC = tests/test_bm.c src/bm_parallel.c src/bm_unicode.c

.PHONY: all clean test verify

all: $(BIN) $(TESTS)

# Lower the pure core to assembly with CompCert. This is the
# verified step: CompCert's machine-checked theorem guarantees
# the assembly behaves as the C source.
$(CORE_ASM): $(CORE_SRC) src/bm_core.h
	$(CCOMP) -S -Isrc -o $@ $(CORE_SRC)

$(CORE_OBJ): $(CORE_ASM)
	$(CC) -c -o $@ $<

# CLI binary: GCC harness + CompCert core.
$(BIN): $(HARNESS_SRC) $(CORE_OBJ)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

# Test binary: same 4,265-check suite, core from CompCert.
$(TESTS): $(TEST_SRC) $(CORE_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

test: $(TESTS)
	./$(TESTS)

# Full verification: suite + CLI comparison against the GCC
# reference binary (path set by GCC_BM, default is the original
# repo's binary). The CLI comparison needs the reference built.
verify: test $(BIN)
	./tests/cli_compare.sh

clean:
	rm -f $(BIN) $(TESTS) $(CORE_OBJ)
