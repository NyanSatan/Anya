#
# Anya main makefile
#

EMBEDDED_PLATFORM_MAP_TOOL := tools/embedded_platform_map
PLATFORM_MAP := platform_map.plist
PLATFORM_MAP_CONSOLIDATED := platform_map_consolidated.plist

VALID_HANDLER_TARGETS := $(shell $(EMBEDDED_PLATFORM_MAP_TOOL) $(PLATFORM_MAP) targets)

BUILD_TAG_BASE_FILE := .tag

ifneq (,$(wildcard $(BUILD_TAG_BASE_FILE)))
	DIRTY := $(shell git diff-files --quiet; echo $$?)
	TAG_RAW := $(shell printf $(shell cat $(BUILD_TAG_BASE_FILE)))

	ifeq ($(DIRTY),0)
		TAG := $(TAG_RAW)
	else
		TAG := $(TAG_RAW)-dirty
	endif
else
	TAG := $(shell printf "Anya-private_build...%s" $(shell TZ=UTC date "+%Y-%m-%d...%H-%M-%S"))
endif

ROOT_DIR := $(realpath .)

BUILD_DIR := build
PACKAGE_DIR := package
PACKAGE_FILE := $(PACKAGE_DIR)/$(TAG).tar.xz

ASTRIS_DIR := astris
ASTRIS_TARGETS := $(addprefix $(ASTRIS_DIR)/,$(shell $(EMBEDDED_PLATFORM_MAP_TOOL) $(PLATFORM_MAP) scripts))

CTL_DIR := ctl
CTL_BUILD_TARGETS := \
	$(CTL_DIR)/build/Anya \
	$(CTL_DIR)/build/libanya.dylib

HANDLER_DIR := handler
HANDLER_BUILD_DIR := $(HANDLER_DIR)/build
HANDLER_FINAL_BUILD_DIR := $(BUILD_DIR)/payloads

PYTHON_DIR := ctl/python

PLATFORM_MAP_CONSOLIDATED_DEST := $(BUILD_DIR)/$(PLATFORM_MAP_CONSOLIDATED)

$(shell echo "TAG=\"$(TAG)\"" > $(PYTHON_DIR)/anya/__tag.py)

$(shell mkdir -p $(BUILD_DIR) $(HANDLER_FINAL_BUILD_DIR))

PACKAGE_DIR_HELPER = mkdir -p $(PACKAGE_DIR)

HANDLER_MAKE_TARGETS := $(addprefix handler_,$(VALID_HANDLER_TARGETS))

.PHONY: all ctl astris python package clean distclean $(HANDLER_MAKE_TARGETS) $(VALID_HANDLER_TARGETS)

all: ctl python
	@echo "%%%%%% all done"

package: all
	@echo "%%%%%% packaging"
	@$(PACKAGE_DIR_HELPER)
	@tar --disable-copyfile --exclude .DS_Store --exclude __pycache__ --exclude $(PLATFORM_MAP_CONSOLIDATED) -cJvf $(PACKAGE_FILE) -C $(BUILD_DIR) .
	@echo "%%%%%% packaging done"

ctl: $(PLATFORM_MAP_CONSOLIDATED_DEST)
	@echo "%%%%%% building the control utility and library"
	@$(MAKE) -C $(CTL_DIR) TAG="$(TAG)" PLATFORM_MAP=$(ROOT_DIR)/$(PLATFORM_MAP_CONSOLIDATED_DEST)
	@cp -a $(CTL_BUILD_TARGETS) $(BUILD_DIR)

$(PLATFORM_MAP_CONSOLIDATED_DEST): $(HANDLER_MAKE_TARGETS) astris
	@$(EMBEDDED_PLATFORM_MAP_TOOL) $(PLATFORM_MAP) consolidate $@

$(HANDLER_MAKE_TARGETS): handler_%: %
	@echo "%%%%%% building USB DFU payload for $<"
	@$(MAKE) -C $(HANDLER_DIR) TARGET=$< TAG="$(TAG)"
	@cp -a $(HANDLER_BUILD_DIR)/*$(subst /,-,$<)*.bin $(HANDLER_FINAL_BUILD_DIR)

astris:
	@echo "%%%%%% copying the Astris scripts"
	@mkdir -p $(BUILD_DIR)/astris
	@cp -a $(ASTRIS_TARGETS) $(BUILD_DIR)/astris

python:
	@echo "%%%%%% copying the Python control utility"
	@cp -a $(PYTHON_DIR) $(BUILD_DIR)

clean:
	@$(MAKE) -C $(CTL_DIR) clean
	@$(MAKE) -C $(HANDLER_DIR) clean
	@rm -rf $(BUILD_DIR)
	@echo "%%%%%% all cleaned"

distclean: clean
	@rm -rf $(PACKAGE_DIR)
