CC ?= clang

TAG ?= "Anya-???"

BUILD_ROOT := build

CFLAGS := -O3
CFLAGS += -arch arm64
CFLAGS += -arch x86_64
CFLAGS += -fobjc-arc
CFLAGS += -fvisibility=hidden
CFLAGS += -mmacosx-version-min=10.13
CFLAGS += -Wno-deprecated-declarations
CFLAGS += -Ililirecovery

LDFLAGS := -framework IOKit
LDFLAGS += -framework CoreFoundation
LDFLAGS += -framework Foundation
LDFLAGS += -Wl,-no_adhoc_codesign

CODESIGN := codesign
CODESIGN_FLAGS := -s - -f -i $(TAG)

ifneq ($(PLATFORM_MAP),)
    LDFLAGS += -Wl,-sectcreate,__TEXT,__platform_map,$(PLATFORM_MAP)
endif

LIB_SOURCES := \
	libanya/utils.c \
	libanya/protocol.c \
	lilirecovery/lilirecovery.c

CTL_SOURCES := \
	$(LIB_SOURCES) \
	map.m \
	batch.m \
	astris.c \
	main.c

RESULT := $(BUILD_ROOT)/Anya
DYLIB_RESULT := $(BUILD_ROOT)/Anya.dylib

DIR_HELPER = mkdir -p $(@D)

.PHONY: all clean

all: $(RESULT) $(DYLIB_RESULT)
	@echo "\tcodesigning"
	@$(CODESIGN) $(CODESIGN_FLAGS) $^
	@echo "%%%% done building"

$(RESULT): $(CTL_SOURCES)
	@echo "\tbuilding anyactl"
	@$(DIR_HELPER)
	@$(CC) $(CFLAGS) $(LDFLAGS) $(CTL_SOURCES) -o $@

$(DYLIB_RESULT): $(LIB_SOURCES)
	@echo "\tbuilding libanya"
	@$(DIR_HELPER)
	@$(CC) $(CFLAGS) -dynamiclib $(LDFLAGS) $(LIB_SOURCES) -o $@

clean:
	@rm -rf $(BUILD_ROOT)
	@echo "%%%% done cleaning"
