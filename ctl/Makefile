CC ?= clang

BUILD_ROOT = build

CFLAGS += -O3
CFLAGS += -arch arm64
CFLAGS += -arch x86_64
CFLAGS += -mmacosx-version-min=10.8
CFLAGS += -Ililirecovery

LDFLAGS += -framework IOKit 
LDFLAGS += -framework CoreFoundation

SOURCES = \
	anyactl.c \
	libanya/anya.c \
	lilirecovery/lilirecovery.c

RESULT = $(BUILD_ROOT)/anyactl

DYLIB_SOURCES = \
	libanya/anya.c \
	lilirecovery/lilirecovery.c

DYLIB_RESULT = $(BUILD_ROOT)/libanya.dylib

DIR_HELPER = mkdir -p $(@D)


all: $(RESULT) $(DYLIB_RESULT)
	@echo "%%%% done building"

$(RESULT): $(SOURCES)
	@echo "\tbuilding anyactl"
	@$(DIR_HELPER)
	@$(CC) $(CFLAGS) $(LDFLAGS) $(SOURCES) -o $@

$(DYLIB_RESULT): $(DYLIB_SOURCES)
	@echo "\tbuilding libanya"
	@$(DIR_HELPER)
	@$(CC) $(CFLAGS) -dynamiclib $(LDFLAGS) $(DYLIB_SOURCES) -o $@

.PHONY: clean

clean:
	@rm -rf $(BUILD_ROOT)
	@echo "%%%% done cleaning"
