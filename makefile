#
# ws2812b SPI makefile
# Philipp Schilk 2021
#

# Compiler + Flags
CC=gcc
LDFLAGS=
CFLAGS=-Wall -Wextra -Wpedantic -g -Isrc -Itest/Unity
DEPFLAGS=-MMD -MP -MF $(BUILDDIR)/$*.d

SOURCES=src/ws2812b.c test/Unity/unity.c 
TESTS=$(wildcard test/*.c)
OBJECTS=$(addprefix $(BUILDDIR)/,$(SOURCES:.c=.o))
RESULTS=$(addprefix $(BUILDDIR)/,$(TESTS:.c=.test))
DEPENDENCIES=$(addprefix $(BUILDDIR)/,$(SOURCES:.c=.d))

BUILDDIR=build

.PHONY: all test test_list_pass clean format

all: test test_list_pass

test: $(RESULTS) 
	@echo
	@echo
	@echo "-----------------------IGNORES:-----------------------"
	@grep -s -r -h --include "*.test" :IGNORE $(BUILDDIR)/ || true
	@echo "-----------------------FAILURES:----------------------"
	@grep -s -r -h --include "*.test" :FAIL $(BUILDDIR)/ || true

test_list_pass: test 
	@echo "-------------------------PASS:------------------------"
	@grep -s -r -h --include "*.test" :PASS $(BUILDDIR)/ || true

clean:
	rm -rf $(BUILDDIR)

# format:
#	python3 scripts/clang_format.py

# Generate results by running all tests:
%.test: %.out FORCE
	@echo Running tests: $<
	-./$< > $@ 2>&1

FORCE: ;

# Link tests:
$(BUILDDIR)/%.out: $(BUILDDIR)/%.o $(OBJECTS)
	$(CC) $^ -o $@

# Compile 
$(BUILDDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $(DEPFLAGS) $*.c -o $@

# Keep dependencies around, make them an explicit target:
$(DEPENDENCIES):

# Keep object files and output files:
.PRECIOUS: $(BUILDDIR)/%.out
.PRECIOUS: $(BUILDDIR)/%.o

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),format)
include $(DEPENDENCIES)
endif
endif
