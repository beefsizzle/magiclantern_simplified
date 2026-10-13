MODEL = M200
FW_VERSION = 100

TOP_DIR = ../..

AUTOEXEC_BASE = 0x800000
FIR_BASE = 0x800120

FIRMWARE_ID     = 0x00000812
MAIN_FIRMWARE_ADDR = 0xE0040000

# user_mem_start from cstart struct: (0xB2A2F & ~7) + 0x800 = 0xB3228,
# plus the remainder added by boot-d678.c ELF alignment fixup = 0xB3230
RESTARTSTART = 0xB3230

# Cortex A9, binaries loaded as Thumb
CFLAGS_PLATFORM = -mthumb -mlong-calls
PLATFORM_ARCH = armv7-a

ML_BOOT_OBJ = boot-d678.o
ML_SRC_EXTRA_OBJS += $(BUILD_DIR)/function_overrides.o $(BUILD_DIR)/lv-vsync.o

include ../Makefile
.DEFAULT_GOAL := $(BUILD_DIR)/cam_complete
