#
# ws2812b SPI makefile
# Philipp Schilk 2021
#

# Compiler + Flags
CC=gcc
LDFLAGS=
CFLAGS=-Wall -Wextra -Wpedantic -Werror=vla -g -Isrc -Itest/Unity
DEPFLAGS=-MMD -MP -MF $(BUILDDIR)/$*.d

SOURCES=src/ws2812b.c test/Unity/unity.c 
TEST_SOURCES=$(wildcard test/*.c)
TESTS=$(addprefix $(BUILDDIR)/,$(TEST_SOURCES:.c=.out))
OBJECTS=$(addprefix $(BUILDDIR)/,$(SOURCES:.c=.o))
DEPENDENCIES=$(addprefix $(BUILDDIR)/,$(SOURCES:.c=.d))
DEPENDENCIES+=$(addprefix $(BUILDDIR)/,$(TEST_SOURCES:.c=.d))

BUILDDIR=build

.PHONY: all test clean format 

all: test

test: $(TESTS)
	-python3 scripts/run_tests.py $(TESTS)

clean:
	rm -rf $(BUILDDIR)

format:
	python3 scripts/clang_format.py

# Link tests:
$(BUILDDIR)/%.out: $(BUILDDIR)/%.o $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@

# Compile sources and test sources
$(BUILDDIR)/%.o: %.c makefile
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $(DEPFLAGS) $*.c -o $@

# Re-generate compile_commands.json using either bear or compiledb
GEN_COMP_COMMANDS_CMD=compiledb make
#GEN_COMP_COMMANDS_CMD=bear -- make
compile_commands: clean
	$(GEN_COMP_COMMANDS_CMD)

# Keep dependencies around, make them an explicit target:
$(DEPENDENCIES):

# Keep object files and output files:
.PRECIOUS: $(BUILDDIR)/%.out
.PRECIOUS: $(BUILDDIR)/%.o

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),format)
ifneq ($(MAKECMDGOALS),compile_commands)
include $(DEPENDENCIES)
endif
endif
endif
