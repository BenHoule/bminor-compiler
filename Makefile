CC        := gcc-14
BIN       := bminor
ROOT_DIR  := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
SRC_DIR   := $(ROOT_DIR)/src
BUILD_DIR := $(ROOT_DIR)/build
TEST_DIR  := $(ROOT_DIR)/test
BOOK_DIR  := $(ROOT_DIR)/book_tests

# -MMD -MP make gcc emit .d files so header edits trigger rebuilds automatically.
CPPFLAGS += -I$(SRC_DIR) -I$(BUILD_DIR)
CFLAGS   ?= -std=c23 -g -Wall -Wextra -Werror
DEPFLAGS := -MMD -MP

# flex/bison emit code we do not control; do not fail the build on its warnings.
# gnu23: flex calls POSIX fileno(), which strict ISO mode hides.
GENFLAGS ?= -std=gnu23 -g -Wall -Wextra -Wno-sign-compare
LDLIBS   ?=
PROFILE  ?= debug

# -O2 for safe optimization (-03 can be overly aggressive)
ifeq ($(PROFILE),release)
CFLAGS += -O2
GENFLAGS += -O2
endif

# Sanitized build for debugging with address and undefined behavior checks
ifeq ($(PROFILE),sanitize)
CFLAGS += -O1 -fsanitize=address,undefined -fno-omit-frame-pointer
GENFLAGS += -O1 -fsanitize=address,undefined -fno-omit-frame-pointer
LDFLAGS += -fsanitize=address,undefined
endif

# Source files
FLEX_SRC  := $(wildcard $(SRC_DIR)/*.flex)
BISON_SRC := $(wildcard $(SRC_DIR)/*.bison)
HAND_SRC  := $(wildcard $(SRC_DIR)/*.c)

# Generated source files
GEN_SRC   := $(patsubst $(SRC_DIR)/%.flex,$(BUILD_DIR)/%.c,$(FLEX_SRC)) \
             $(patsubst $(SRC_DIR)/%.bison,$(BUILD_DIR)/%.c,$(BISON_SRC))
BISON_HEADERS := $(patsubst $(SRC_DIR)/%.bison,$(BUILD_DIR)/%.tab.h,$(BISON_SRC))

# Object files
OBJS      := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(HAND_SRC)) \
             $(GEN_SRC:.c=.o)

# Make Commands
.PHONY: all test clean valgrind unit release sanitize analyze format-check

# Prevent generated source files from being deleted after compilation
.SECONDARY: $(GEN_SRC)

# Default target
all: $(BIN)

# Rebuild objects when checked-in build flags/rules change.
$(OBJS): $(ROOT_DIR)/Makefile

# Build the binary from object files
$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

# Create the build directory
$(BUILD_DIR):
	@mkdir -p $@

# Build object files from hand-written source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) -c -o $@ $<

$(BUILD_DIR)/hash_table.o: $(SRC_DIR)/hash_table.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) -c -o $@ $<

# Build object files from generated source files
$(BUILD_DIR)/%.o: $(BUILD_DIR)/%.c
	$(CC) $(CPPFLAGS) $(GENFLAGS) $(DEPFLAGS) -c -o $@ $<

# Generate bison source files
# -v/--report-file produce build/bminor.output.
$(BUILD_DIR)/%.c $(BUILD_DIR)/%.tab.h &: $(SRC_DIR)/%.bison | $(BUILD_DIR)
	bison --defines=$(BUILD_DIR)/$*.tab.h --output=$(BUILD_DIR)/$*.c \
	      -v --report-file=$(BUILD_DIR)/$*.output $<

# Generate flex source files
$(BUILD_DIR)/%.c: $(SRC_DIR)/%.flex | $(BUILD_DIR)
	flex --outfile=$@ $<

# Scanner and driver both include the bison-generated header, so it must exist first.
$(BUILD_DIR)/scanner.o $(BUILD_DIR)/main.o: $(BISON_HEADERS)

# Scanner and string utility unit tests
FRONTEND_OBJS := $(filter-out $(BUILD_DIR)/main.o,$(OBJS))
$(BUILD_DIR)/scanner_unit_tests: $(TEST_DIR)/scanner/unit_tests.c $(FRONTEND_OBJS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(BUILD_DIR)/programs_unit_tests: $(TEST_DIR)/programs/unit_tests.c $(FRONTEND_OBJS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(BUILD_DIR)/runtime_unit_tests: $(TEST_DIR)/runtime/unit_tests.c $(SRC_DIR)/library.c $(SRC_DIR)/library.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $(TEST_DIR)/runtime/unit_tests.c $(SRC_DIR)/library.c $(LDLIBS)

unit: $(BUILD_DIR)/scanner_unit_tests $(BUILD_DIR)/programs_unit_tests $(BUILD_DIR)/runtime_unit_tests
	@$(BUILD_DIR)/scanner_unit_tests
	@$(BUILD_DIR)/programs_unit_tests
	@$(BUILD_DIR)/runtime_unit_tests > $(BUILD_DIR)/runtime-print.out
	@diff -u $(TEST_DIR)/runtime/print.expected $(BUILD_DIR)/runtime-print.out
	@$(BUILD_DIR)/runtime_unit_tests overflow > $(BUILD_DIR)/runtime-overflow.out 2>&1; \
	  status=$$?; test $$status -eq 1 || exit 1; \
	  diff -u $(TEST_DIR)/runtime/overflow.expected $(BUILD_DIR)/runtime-overflow.out

# Test and valgrind targets
test: $(BIN) unit
	@BMINOR=$(abspath $(BIN)) $(TEST_DIR)/runtests.sh
	@BMINOR=$(abspath $(BIN)) $(TEST_DIR)/printer/roundtrip.sh
	@BMINOR=$(abspath $(BIN)) $(TEST_DIR)/resolver/positions.sh
	@BMINOR=$(abspath $(BIN)) $(TEST_DIR)/programs/run.sh
	@$(BOOK_DIR)/run-tests.sh $(abspath $(BIN)) -scan $(BOOK_DIR)/scanner
	@$(BOOK_DIR)/run-tests.sh $(abspath $(BIN)) -parse $(BOOK_DIR)/parser
	@$(BOOK_DIR)/run-tests.sh $(abspath $(BIN)) -print $(BOOK_DIR)/printer
	@$(BOOK_DIR)/run-tests.sh $(abspath $(BIN)) -typecheck $(BOOK_DIR)/typecheck

valgrind: $(BIN) $(BUILD_DIR)/scanner_unit_tests $(BUILD_DIR)/programs_unit_tests $(BUILD_DIR)/runtime_unit_tests
	@valgrind --quiet --error-exitcode=125 --leak-check=full $(BUILD_DIR)/scanner_unit_tests
	@valgrind --quiet --error-exitcode=125 --leak-check=full $(BUILD_DIR)/programs_unit_tests
	@valgrind --quiet --error-exitcode=125 --leak-check=full $(BUILD_DIR)/runtime_unit_tests
	@BMINOR=$(abspath $(BIN)) BMINOR_VALGRIND=1 $(TEST_DIR)/runtests.sh

release:
	$(MAKE) PROFILE=release BUILD_DIR=$(ROOT_DIR)/build/release BIN=$(ROOT_DIR)/build/release/bminor all

sanitize:
	$(MAKE) PROFILE=sanitize BUILD_DIR=$(ROOT_DIR)/build/sanitize BIN=$(ROOT_DIR)/build/sanitize/bminor test

ANALYSIS_OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/analyze/%.o,$(HAND_SRC) $(SRC_DIR)/library.c)
analyze: $(ANALYSIS_OBJS)

$(BUILD_DIR)/analyze/%.o: $(SRC_DIR)/%.c $(BISON_HEADERS) $(wildcard $(SRC_DIR)/*.h) $(ROOT_DIR)/Makefile
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -fanalyzer -c $< -o $@

format-check:
	clang-format --dry-run --Werror $(HAND_SRC) $(wildcard $(SRC_DIR)/*.h)

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(BIN)
	find $(BOOK_DIR) -type f -name '*.bminor.out' -delete
	find $(ROOT_DIR)/test -type d -name out -prune -exec rm -rf {} +

# Include dependency files generated by the compiler for header tracking
# Ensures all dependencies are tracked and header edits trigger rebuilds
-include $(OBJS:.o=.d)
