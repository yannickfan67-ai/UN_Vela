CC ?= clang
CFLAGS := -std=c11 -Wall -Wextra -Werror -Iinclude -Ivendor/aster/include
BUILD := build

.PHONY: all clean check-x86_64 check-aarch64 check-riscv64 check-all
all: check-x86_64

$(BUILD):
	mkdir -p $(BUILD)

check-x86_64: | $(BUILD)
	$(CC) $(CFLAGS) -c src/vela.c -o $(BUILD)/vela-x86_64.o

check-aarch64: | $(BUILD)
	$(CC) -target aarch64-none-elf -ffreestanding $(CFLAGS) -c src/vela.c -o $(BUILD)/vela-aarch64.o

check-riscv64: | $(BUILD)
	$(CC) -target riscv64-none-elf -ffreestanding $(CFLAGS) -c src/vela.c -o $(BUILD)/vela-riscv64.o

check-all: check-x86_64 check-aarch64 check-riscv64

clean:
	rm -rf $(BUILD)
