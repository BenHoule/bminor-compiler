CLANG ?= clang
TARGET := x86_64-unknown-linux-gnu
CFLAGS ?= -std=c17 -Wall -Wextra -Werror
COMMON_FLAGS := --target=$(TARGET) -O0 -fno-omit-frame-pointer

SOURCES := $(wildcard *.c)
LLVM := $(patsubst %.c,llvm/%.ll,$(SOURCES))
X86 := $(patsubst %.c,x86/%.s,$(SOURCES))

.PHONY: all llvm x86 clean
.DELETE_ON_ERROR:
.SECONDARY: $(LLVM)

all: $(LLVM) $(X86)
llvm: $(LLVM)
x86: $(X86)

llvm/%.ll: %.c Makefile
	@mkdir -p $(@D)
	$(CLANG) $(CFLAGS) $(COMMON_FLAGS) -fno-inline -fno-builtin \
		-fno-stack-protector -fno-discard-value-names -gline-tables-only \
		-S -emit-llvm $< -o $@

x86/%.s: llvm/%.ll Makefile
	@mkdir -p $(@D)
	$(CLANG) $(COMMON_FLAGS) -masm=att -fverbose-asm -S $< -o $@

clean:
	rm -f $(LLVM) $(X86)
