#
# Dumpster fire of a makefile
# Someday I'll fix it, I promise
#

VALID_HANDLER_TARGETS ?= Skye/A0 Cyprus/A0 Cyprus/B0 Cyprus/B1 M9/B0_B1 Aruba/A1 Cebu Sicily/A0 Sicily/B0 Sicily/B1 Tonga/B1 Ellis/A0 Ellis/B0 Staten/B1 Crete/A0
PYTHON ?= python3


BUILD_DIR = build

PACKAGE_DIR = package
PACKAGE_FILE = $(PACKAGE_DIR)/Anya.zip

ASTRIS_DIR = astris
ASTRIS_TARGET = $(ASTRIS_DIR)/anya.ax
ASTRIS_CRETE_TARGET = $(ASTRIS_DIR)/anya_crete.ax

CTL_DIR = ctl
CTL_BUILD_TARGET = $(CTL_DIR)/build/anyactl
CTL_BUILD_TARGET_DYLIB = $(CTL_DIR)/build/libanya.dylib

HANDLER_BUILDER = tools/build_targets.py
HANDLER_DIR = handler
HANDLER_BUILD_DIR = $(HANDLER_DIR)/build
HANDLER_FINAL_BUILD_DIR = $(BUILD_DIR)/payloads

PYTHON_DIR = ctl/python

DIR_HELPER = mkdir -p $(BUILD_DIR) $(HANDLER_FINAL_BUILD_DIR)
PACKAGE_DIR_HELPER = mkdir -p $(PACKAGE_DIR)

.PHONY: all astris ctl handler python package clean

all: astris ctl handler python
	@echo "%%%%%% all done"

package: astris ctl handler python
	@echo "%%%%%% packaging"
	@$(PACKAGE_DIR_HELPER)
	@rm -rf $(PACKAGE_FILE)
	@zip -x "*.DS_Store*" -x "*__pycache__*" -r9 $(PACKAGE_FILE) $(BUILD_DIR)/*
	@echo "%%%%%% packaging done"

astris:
	@$(DIR_HELPER)
	@echo "%%%%%% copying the Astris scripts"
	@cp -a $(ASTRIS_TARGET) $(BUILD_DIR)
	@cp -a $(ASTRIS_CRETE_TARGET) $(BUILD_DIR)

ctl:
	@echo "%%%%%% building the control utility and library"
	@make -C $(CTL_DIR)
	@$(DIR_HELPER)
	@echo "%%%%%% copying the control utility and library"
	@cp -a $(CTL_BUILD_TARGET) $(BUILD_DIR)
	@cp -a $(CTL_BUILD_TARGET_DYLIB) $(BUILD_DIR)

handler:
	@$(DIR_HELPER)
	@echo "%%%%%% building the handlers payloads for: $(VALID_HANDLER_TARGETS)"
	@$(PYTHON) $(HANDLER_BUILDER) $(HANDLER_DIR) $(HANDLER_FINAL_BUILD_DIR) $(VALID_HANDLER_TARGETS)

python:
	@echo "%%%%%% copying the Python control utility"
	@cp -a $(PYTHON_DIR) $(BUILD_DIR)

clean:
	@make -C $(CTL_DIR) clean
	@make -C $(HANDLER_DIR) clean
	@rm -rf $(BUILD_DIR)
	@rm -rf $(PACKAGE_DIR)
	@echo "%%%%%% all cleaned"
