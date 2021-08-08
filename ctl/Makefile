CC ?= clang
AR ?= ar

BUILD_ROOT = build
BUILD_LIB_ROOT = $(BUILD_ROOT)/lib
SOURCE_ROOT = src

LDFLAGS = -L$(BUILD_LIB_ROOT)
LDFLAGS += -lanya
LDFLAGS += -lirecovery-1.0

SOURCE = $(SOURCE_ROOT)/anyactl.c
RESULT = $(BUILD_ROOT)/anyactl

DIR_HELPER = mkdir -p $(@D)

.DEFAULT_GOAL := all

include $(SOURCE_ROOT)/libanya/library.mk

all: $(LIBANYA_STATIC) $(RESULT)
	@echo "%%%% done building"

staticlib: $(LIBANYA_STATIC)

$(RESULT): $(LIBANYA_STATIC) $(SOURCE)
	@echo "\tbuilding anyactl"
	@$(DIR_HELPER)
	@$(CC) $(LDFLAGS) $(SOURCE) -o $@

.PHONY: clean

clean:
	@rm -rf $(BUILD_ROOT)/*
	@echo "%%%% done cleaning"
