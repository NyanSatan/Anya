#
# User changeable vars (via env)
#

ARM_CC ?= xcrun -sdk iphoneos clang
VMACHO ?= vmacho
PYTHON ?= python3

#
# Not user changeable
#

TAG ?= "Anya-???"

TARGETS_DIR := targets

ifneq ($(MAKECMDGOALS),clean)
	ifeq ($(TARGET),)
        $(error no TARGET specified)
	else
		include $(TARGETS_DIR)/$(TARGET)/target.mk
	endif
endif

TARGET_KBAG_BUFFER_SIZE := 0x8000	# must be synchronized with ANYA_MAX_PACKET_SIZE in libanya/anya.c

TARGET_DATA_START := $(shell printf "0x%x" `echo $$(( $(TARGET_LOADADDR) + $(TARGET_KBAG_BUFFER_SIZE)))`)

TARGET_BUILD_STRING := "Anya USB DFU Handler for $(TARGET_NAME), $(TAG)"

OPTIONS := -DTARGET_TRAMPOLINE=$(TARGET_TRAMPOLINE) \
		   -DTARGET_LOADADDR=$(TARGET_LOADADDR) \
		   -DTARGET_KBAG_BUFFER_SIZE=$(TARGET_KBAG_BUFFER_SIZE) \
		   -DTARGET_HANDLE_INTERFACE_REQUEST=$(TARGET_HANDLE_INTERFACE_REQUEST) \
		   -DTARGET_USB_CORE_DO_TRANSFER=$(TARGET_USB_CORE_DO_TRANSFER) \
		   -DTARGET_USB_TOTAL_RECEIVED=$(TARGET_USB_TOTAL_RECEIVED) \
		   -DTARGET_AES_CRYPTO_CMD=$(TARGET_AES_CRYPTO_CMD) \
		   -DTARGET_PLATFORM_RESET=$(TARGET_PLATFORM_RESET)

ifeq ($(TARGET_CAN_HAVE_SEP_SUPPORT),true)
	OPTIONS += -DWITH_SEP=1 \
			   -DTARGET_SEP_CORESIGHT_BASE=$(TARGET_SEP_CORESIGHT_BASE) \
			   -DTARGET_SEP_AES_BASE=$(TARGET_SEP_AES_BASE) \
			   -DTARGET_PLATFORM_GET_CPFM=$(TARGET_PLATFORM_GET_CPFM) \
			   -DTARGET_SYSTEM_TIME=$(TARGET_SYSTEM_TIME)

	ifeq ($(TARGET_HAS_SKYE_SEP_AES),true)
		OPTIONS += -DWITH_SKYE_SEP_AES=1
	endif

	ifeq ($(TARGET_HAS_ALCATRAZ_SEP_AES),true)
		OPTIONS += -DWITH_ALCATRAZ_SEP_AES=1
	endif

	ifeq ($(TARGET_NEEDS_DPA_HACK),true)
		OPTIONS += -DWITH_DPA_HACK=1
	endif

	ifeq ($(TARGET_HAS_SEP32),true)
		OPTIONS += -DWITH_SEP32=1
	endif
endif

ifeq ($(TARGET_HAS_FIREBLOOM),true)
	OPTIONS += -DWITH_FIREBLOOM=1 \
			   -DTARGET_FIREBLOOM_KBAG_TYPE=$(TARGET_FIREBLOOM_KBAG_TYPE) \
			   -DTARGET_FIREBLOOM_BYTE_TYPE=$(TARGET_FIREBLOOM_BYTE_TYPE)
endif

OPTIONS += -DTARGET_BUILD_STRING=\"$(TARGET_BUILD_STRING)\"

ifeq ($(TARGET_HAS_ARMV7),true)
	ARCH := -arch armv7
else
	ARCH := -arch arm64
endif

CFLAGS := $(ARCH)
CFLAGS += $(OPTIONS)
CFLAGS += -fno-builtin -fno-pie
CFLAGS += -Os
CFLAGS += -MMD
CFLAGS += -fno-stack-protector

LDFLAGS := $(ARCH)
LDFLAGS += -Wl,-no_pie
LDFLAGS += -nostdlib
LDFLAGS += -static
LDFLAGS += -Wl,-preload
LDFLAGS += -Wl,-order_file,misc/sym_order.txt
LDFLAGS += -Wl,-e,_anya_handle_interface_request
LDFLAGS += -Wl,-seg1addr,$(TARGET_TRAMPOLINE)
LDFLAGS += -Wl,-segaddr,__DATA,$(TARGET_DATA_START)

PROJECT_NAME := anya_handler

SOURCE_ROOT := src
BUILD_ROOT := build
CURRENT_ROOT := $(BUILD_ROOT)/$(TARGET_NAME)/$(SOURCE_ROOT)

OBJECTS := $(CURRENT_ROOT)/main.o $(CURRENT_ROOT)/ap.o

ifeq ($(TARGET_HAS_ARMV7),true)
	OBJECTS += $(CURRENT_ROOT)/copy32.o
else
	OBJECTS += $(CURRENT_ROOT)/copy64.o
endif

ifeq ($(TARGET_CAN_HAVE_SEP_SUPPORT),true)
	OBJECTS += $(CURRENT_ROOT)/time.o $(CURRENT_ROOT)/coresight.o $(CURRENT_ROOT)/sep.o

	ifneq ($(TARGET_HAS_SEP32),true)
		OBJECTS += $(CURRENT_ROOT)/dbgwrap.o
	endif
endif

TARGET_BASENAME := $(PROJECT_NAME).$(TARGET_NAME)
TARGET_MACHO := $(BUILD_ROOT)/$(TARGET_BASENAME).o
TARGET_BIN := $(BUILD_ROOT)/$(TARGET_BASENAME).bin

DIR_HELPER = mkdir -p $(@D)

.PHONY: all clean

all: $(TARGET_BIN)
	@echo "%%%% done building"

$(TARGET_BIN): $(TARGET_MACHO)
	@echo "\textracting binary"
	@$(DIR_HELPER)
	@$(VMACHO) -f $< $@
	@echo "\tpadding binary"
	@dd status=none if=/dev/zero bs=1 count=$$(((8 - ($$(stat -f %z $@) % 8)) % 8)) >> $@

$(TARGET_MACHO): $(OBJECTS)
	@echo "\tlinking"
	@$(DIR_HELPER)
	@$(ARM_CC) $(LDFLAGS) $^ -o $@

$(CURRENT_ROOT)/%.o: $(SOURCE_ROOT)/%.c
	@echo "\tcompiling C: $<"
	@$(DIR_HELPER)
	@$(ARM_CC) $(CFLAGS) $(OPTIONS) -c $< -o $@

$(CURRENT_ROOT)/%.o: $(SOURCE_ROOT)/%.S
	@echo "\tcompiling ASM: $<"
	@$(DIR_HELPER)
	@$(ARM_CC) $(CFLAGS) $(OPTIONS) -c $< -o $@

clean:
	@rm -rf $(BUILD_ROOT)
	@echo "%%%% done cleaning"

-include $(OBJECTS:.o=.d)
