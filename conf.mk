BUILD_DIR := $(PWD)/build

CC     := gcc
CFLAGS := -Wall -Wextra 

SRA32_CC      := sra32-elf-gcc
SRA32_CFLAGS  := -Wall -Wextra -O2 -ffreestanding -fno-builtin -fno-stack-protector -Iinclude
SRA32_LDFLAGS := -nostdlib -nostartfiles -nodefaultlibs -Wl,--noinhibit-exec