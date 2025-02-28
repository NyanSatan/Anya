#
# Anya main makefile
#

VALID_HANDLER_TARGETS ?= \
	Alcatraz/A0 Alcatraz/B0 \
	Gibraltar/B0 \
	Skye/A0 \
	Cyprus/A0 Cyprus/B0 Cyprus/B1 \
	M9/B0_B1 \
	Aruba/A1 \
	Cebu \
	Sicily/A0 Sicily/B0 Sicily/B1 \
	Turks/A0 Turks/B0 \
	Tonga/B1 \
	Ellis/A0 Ellis/B0_B1 \
	Staten/B1 \
	Crete/A0 Crete/B1 \
	Coll/A0

PYTHON ?= python3

BUILD_TAG_DB := anya_tag_db.json

TAG := $(shell $(PYTHON) polinatag.py generate . $(BUILD_TAG_DB))

BUILD_DIR := build

PACKAGE_DIR := package

ifeq (,$(findstring private_build,$(TAG)))
	PACKAGE_FILE := $(PACKAGE_DIR)/$(TAG).tar.xz
else
	PACKAGE_FILE := $(PACKAGE_DIR)/Anya-private-$(shell TZ=UTC date "+%Y-%m-%d_%H-%M-%S").tar.xz
endif

ASTRIS_DIR := astris
ASTRIS_TARGETS := \
	$(ASTRIS_DIR)/anya.ax \
	$(ASTRIS_DIR)/anya_4k.ax \
	$(ASTRIS_DIR)/anya_crete.ax \
	$(ASTRIS_DIR)/anya_coll.ax

CTL_DIR := ctl
CTL_BUILD_TARGETS := \
	$(CTL_DIR)/build/anyactl \
	$(CTL_DIR)/build/libanya.dylib

HANDLER_DIR := handler
HANDLER_BUILD_DIR := $(HANDLER_DIR)/build
HANDLER_FINAL_BUILD_DIR := $(BUILD_DIR)/payloads

PYTHON_DIR := ctl/python

$(shell echo "TAG=\"$(TAG)\"" > $(PYTHON_DIR)/anya/__tag.py)

$(shell mkdir -p $(BUILD_DIR) $(HANDLER_FINAL_BUILD_DIR))

PACKAGE_DIR_HELPER = mkdir -p $(PACKAGE_DIR)

HANDLER_TARGETS := $(addprefix handler_,$(VALID_HANDLER_TARGETS))

.PHONY: all astris ctl python package clean distclean $(HANDLER_TARGETS) $(VALID_HANDLER_TARGETS)

all: $(HANDLER_TARGETS) astris ctl python
	@$(shell $(PYTHON) polinatag.py commit $(BUILD_TAG_DB))
	@echo "%%%%%% all done"

package: all
	@echo "%%%%%% packaging"
	@$(PACKAGE_DIR_HELPER)
	@tar --disable-copyfile --exclude .DS_Store --exclude __pycache__ -cJvf $(PACKAGE_FILE) -C $(BUILD_DIR) .
	@echo "%%%%%% packaging done"

astris:
	@echo "%%%%%% copying the Astris scripts"
	@cp -a $(ASTRIS_TARGETS) $(BUILD_DIR)

ctl:
	@echo "%%%%%% building the control utility and library"
	@$(MAKE) -C $(CTL_DIR) TAG="$(TAG)"
	@echo "%%%%%% copying the control utility and library"
	@cp -a $(CTL_BUILD_TARGETS) $(BUILD_DIR)

$(HANDLER_TARGETS): handler_%: %
	@echo "%%%%%% building USB DFU payload for $<"
	@$(MAKE) -C $(HANDLER_DIR) TARGET=$< TAG="$(TAG)"
	@cp -a $(HANDLER_BUILD_DIR)/*$(subst $<,/,-)*.bin $(HANDLER_FINAL_BUILD_DIR)

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
