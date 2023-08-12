#
# User changeable vars (via env)
#

ARM_CC ?= xcrun -sdk iphoneos clang
VMACHO ?= vmacho

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

TARGET_KBAG_BUFFER_SIZE = 0x8000

TARGET_DATA_START = $(shell printf "0x%x" `echo $$(( $(TARGET_LOADADDR) + $(TARGET_KBAG_BUFFER_SIZE)))`)

TARGET_BUILD_STRING = "Anya DFU Handler for $(TARGET_NAME), built on $(shell TZ=UTC date "+%Y-%m-%d %H:%M:%S")"

OPTIONS = -DTARGET_BUILD_STRING=\"$(TARGET_BUILD_STRING)\" \
		  -DTARGET_TRAMPOLINE=$(TARGET_TRAMPOLINE) \
		  -DTARGET_LOADADDR=$(TARGET_LOADADDR) \
		  -DTARGET_KBAG_BUFFER_SIZE=$(TARGET_KBAG_BUFFER_SIZE) \
		  -DTARGET_HANDLE_INTERFACE_REQUEST=$(TARGET_HANDLE_INTERFACE_REQUEST) \
		  -DTARGET_USB_CORE_DO_TRANSFER=$(TARGET_USB_CORE_DO_TRANSFER) \
		  -DTARGET_USB_TOTAL_RECEIVED=$(TARGET_USB_TOTAL_RECEIVED) \
		  -DTARGET_AES_CRYPTO_CMD=$(TARGET_AES_CRYPTO_CMD) \
		  -DTARGET_PLATFORM_RESET=$(TARGET_PLATFORM_RESET)


CFLAGS += $(OPTIONS)
CFLAGS += -fno-builtin
CFLAGS += -Os
CFLAGS += -MMD
CFLAGS += -arch arm64


LDFLAGS = -nostdlib
LDFLAGS += -static
LDFLAGS += -Wl,-preload
LDFLAGS += -Wl,-order_file,misc/sym_order.txt
LDFLAGS += -Wl,-e,_anya_handle_interface_request
LDFLAGS += -Wl,-seg1addr,$(TARGET_TRAMPOLINE)
LDFLAGS += -Wl,-segaddr,__DATA,$(TARGET_DATA_START)

PROJECT_NAME = anya_handler

SOURCE_ROOT = src
SOURCE = $(SOURCE_ROOT)/anya_handler.c $(SOURCE_ROOT)/copy.S

BUILD_ROOT = build
TARGET_BASENAME = $(PROJECT_NAME).$(TARGET_NAME)
TARGET_MACHO = $(BUILD_ROOT)/$(TARGET_BASENAME).o
TARGET_BIN = $(BUILD_ROOT)/$(TARGET_BASENAME).bin

DIR_HELPER = mkdir -p $(@D)

all: $(TARGET_BIN)
	@echo "%%%% done building"

$(TARGET_BIN): $(TARGET_MACHO)
	@echo "\textracting binary"
	@$(DIR_HELPER)
	@$(VMACHO) -f $< $@

$(TARGET_MACHO): $(SOURCE)
	@echo "\tbuilding"
	@$(DIR_HELPER)
	@$(ARM_CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

.PHONY: clean

clean:
	$(shell rm -rf $(BUILD_ROOT))
	@echo "%%%% done cleaning"

-include $(TARGET_OBJ:.o=.d)
