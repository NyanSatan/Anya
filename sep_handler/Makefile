#
# User changeable vars (via env)
#

TOOLCHAIN ?= /opt/arm-arm64-llvm-11-toolchain/bin

ARM_CC ?= $(TOOLCHAIN)/clang
ARM_LD ?= $(TOOLCHAIN)/ld.lld
ARM_OBJCOPY ?= $(TOOLCHAIN)/llvm-objcopy

#
# Not user changeable
#

TARGETS_DIR = targets

ifneq ($(MAKECMDGOALS),clean)
ifeq ($(TARGET),)
$(error no TARGET specified)
else
include $(TARGETS_DIR)/$(TARGET)/target.mk
endif
endif


OPTIONS = -DTARGET_TRAMPOLINE=$(TARGET_TRAMPOLINE) \
		  -DTARGET_MAILBOX_GET_MESSAGE=$(TARGET_MAILBOX_GET_MESSAGE) \
		  -DTARGET_MAILBOX_SEND_MESSAGE=$(TARGET_MAILBOX_SEND_MESSAGE) \
		  -DTARGET_MAILBOX_DISCARD=$(TARGET_MAILBOX_DISCARD) \
		  -DTARGET_MAILBOX_PENDING=$(TARGET_MAILBOX_PENDING) \
		  -DTARGET_MEMCPY=$(TARGET_MEMCPY) \
		  -DTARGET_BZERO=$(TARGET_BZERO) \
		  -DTARGET_AES_BASE=$(TARGET_AES_BASE)


CFLAGS += $(OPTIONS)
CFLAGS += -fno-builtin
CFLAGS += -Os
CFLAGS += -MMD

ifneq ($(TARGET_HAS_PAC),true)
	CFLAGS += -target arm64
else
	CFLAGS += -target arm64e
	CFLAGS += -mbranch-protection=pac-ret+b-key+leaf
endif

LDFLAGS = -nostdlib
LDFLAGS += -Tmisc/script.ld
LDFLAGS += -T$(TARGETS_DIR)/$(TARGET)/memory.ld


PROJECT_NAME = anya_sep_handler

SOURCE_ROOT = src
SOURCE = $(SOURCE_ROOT)/anya_sep_handler.c

BUILD_ROOT = build
TARGET_BASENAME = $(PROJECT_NAME).$(TARGET_NAME)
TARGET_OBJ = $(BUILD_ROOT)/$(TARGET_BASENAME).o
TARGET_ELF = $(BUILD_ROOT)/$(TARGET_BASENAME).elf
TARGET_BIN = $(BUILD_ROOT)/$(TARGET_BASENAME).bin

DIR_HELPER = mkdir -p $(@D)

all: $(TARGET_BIN)
	@echo "%%%% done building"

$(TARGET_BIN): $(TARGET_ELF)
	@echo "\textracting binary"
	@$(DIR_HELPER)
	@$(ARM_OBJCOPY) --remove-section .note.gnu.property -O binary $< $@

$(TARGET_ELF): $(TARGET_OBJ)
	@echo "\tlinking"
	@$(DIR_HELPER)
	@$(ARM_LD) -o $@ $(LDFLAGS) $<

$(TARGET_OBJ): $(SOURCE)
	@echo "\tbuilding"
	@$(DIR_HELPER)
	@$(ARM_CC) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
	$(shell rm -rf $(BUILD_ROOT)/*)
	@echo "%%%% done cleaning"

-include $(TARGET_OBJ:.o=.d)
