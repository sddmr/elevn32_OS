ifeq ($(TOOLCHAIN), llvm)
    CXX := clang++
    LD := ld.lld
else ifdef TOOLCHAIN_PREFIX
    CXX := $(TOOLCHAIN_PREFIX)g++
    LD := $(TOOLCHAIN_PREFIX)ld
else
    CXX := x86_64-elf-g++
    LD := x86_64-elf-ld
endif

NASM := nasm

CXXFLAGS := \
    -std=c++20 \
    -ffreestanding \
    -fno-stack-protector \
    -fno-stack-check \
    -fno-exceptions \
    -fno-rtti \
    -fno-lto \
    -fno-PIC \
    -m64 \
    -march=x86-64 \
    -mno-80387 \
    -mno-mmx \
    -mno-sse \
    -mno-sse2 \
    -mno-red-zone \
    -mcmodel=kernel \
    -Wall \
    -Wextra \
    -Isrc

NASMFLAGS := -f elf64

LDFLAGS := \
    -nostdlib \
    -static \
    -z max-page-size=0x1000 \
    -T linker.ld

CXXSRC := $(wildcard src/*.cpp)
ASMSRC := $(wildcard src/*.asm)
OBJ := $(CXXSRC:.cpp=.o) $(ASMSRC:.asm=.asm.o)

.PHONY: all clean

all: kernel.elf

kernel.elf: $(OBJ)
	$(LD) $(LDFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.asm.o: %.asm
	$(NASM) $(NASMFLAGS) $< -o $@

clean:
	rm -f $(OBJ) kernel.elf
