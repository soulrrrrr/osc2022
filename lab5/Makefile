#
# Copyright (C) 2018 bzt (bztsrc@github)
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation
# files (the "Software"), to deal in the Software without
# restriction, including without limitation the rights to use, copy,
# modify, merge, publish, distribute, sublicense, and/or sell copies
# of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
# HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
#
#

ARMGNU ?= aarch64-linux-gnu

SRC_DIR = src
BUILD_DIR = obj

C_FILES = $(wildcard $(SRC_DIR)/*.c)
ASM_FILES = $(wildcard $(SRC_DIR)/*.S)
OBJ_FILES = $(C_FILES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%_c.o)
OBJ_FILES += $(ASM_FILES:$(SRC_DIR)/%.S=$(BUILD_DIR)/%_s.o)
DEP_FILES = $(OBJ_FILES:%.o=%.d)
-include $(DEP_FILES)

CFLAGS = -Wall -ffreestanding -nostdlib -nostartfiles -Iinclude -mgeneral-regs-only
ASMFLAGS = -Iinclude 

all: clean kernel8.img

$(BUILD_DIR)/%_c.o: $(SRC_DIR)/%.c
	$(ARMGNU)-gcc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%_s.o: $(SRC_DIR)/%.S
	$(ARMGNU)-gcc $(ASMFLAGS) -c $< -o $@

kernel8.img: $(OBJ_FILES)
	aarch64-linux-gnu-ld -nostdlib -nostartfiles $(OBJ_FILES) -T linker.ld -o kernel8.elf
	aarch64-linux-gnu-objcopy -O binary kernel8.elf kernel8.img

clean:
	rm kernel8.elf *.o >/dev/null 2>/dev/null || true
	rm -rf $(BUILD_DIR)/* || true

run:
	qemu-system-aarch64 -M raspi3 -kernel kernel8.img -serial null -serial stdio -display none

cpio:
	qemu-system-aarch64 -M raspi3 -kernel kernel8.img -serial null -serial stdio -initrd initramfs.cpio

debug:
	qemu-system-aarch64 -M raspi3 -kernel kernel8.img -serial null -serial stdio -display none -S -s

