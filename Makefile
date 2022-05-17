#
# Dumpster fire of a makefile
# Someday I'll fix it, I promise
#

VALID_HANDLER_TARGETS ?= M9/B0_B1 Cyprus/B1 Cebu/B0_B1 Sicily/A0 Cyprus/B0
VALID_SEP_HANDLER_TARGETS ?= M9/B0_B1 Cyprus/B1
PYTHON ?= python3


BUILD_DIR = build

PACKAGE_DIR = package
PACKAGE_FILE = $(PACKAGE_DIR)/Anya.zip

ASTRIS_DIR = astris
ASTRIS_TARGET = $(ASTRIS_DIR)/anya.ax

CTL_DIR = ctl
CTL_BUILD_TARGET = $(CTL_DIR)/build/anyactl

HANDLER_BUILDER = tools/build_targets.py
HANDLER_DIR = handler
HANDLER_BUILD_DIR = $(HANDLER_DIR)/build
HANDLER_FINAL_BUILD_DIR = $(BUILD_DIR)/payloads

SEP_HANDLER_DIR = sep_handler
SEP_HANDLER_BUILD_DIR = $(SEP_HANDLER_DIR)/build
SEP_HANDLER_FINAL_BUILD_DIR = $(BUILD_DIR)/sep_payloads

PYTHON_DIR = python

DIR_HELPER = mkdir -p $(BUILD_DIR) $(HANDLER_FINAL_BUILD_DIR) $(SEP_HANDLER_FINAL_BUILD_DIR)
PACKAGE_DIR_HELPER = mkdir -p $(PACKAGE_DIR)

.PHONY: all astris ctl handler sep_handler python package clean

all: astris ctl handler sep_handler python
	@echo "%%%%%% all done"

package: astris ctl handler sep_handler python
	@echo "%%%%%% packaging"
	@$(PACKAGE_DIR_HELPER)
	@rm -rf $(PACKAGE_FILE)
	@zip -x "*.DS_Store*" -x "*__pycache__*" -r9 $(PACKAGE_FILE) $(BUILD_DIR)/*
	@echo "%%%%%% packaging done"

astris:
	@$(DIR_HELPER)
	@echo "%%%%%% copying the Astris script"
	@cp -a $(ASTRIS_TARGET) $(BUILD_DIR)

ctl:
	@echo "%%%%%% building the control utility"
	@make -C $(CTL_DIR)
	@$(DIR_HELPER)
	@echo "%%%%%% copying the control utility"
	@cp -a $(CTL_BUILD_TARGET) $(BUILD_DIR)

handler:
	@$(DIR_HELPER)
	@echo "%%%%%% building the handlers payloads for: $(VALID_HANDLER_TARGETS)"
	@$(PYTHON) $(HANDLER_BUILDER) $(HANDLER_DIR) $(HANDLER_FINAL_BUILD_DIR) $(VALID_HANDLER_TARGETS)

sep_handler:
	@$(DIR_HELPER)
	@echo "%%%%%% building the SEP handlers payloads for: $(VALID_SEP_HANDLER_TARGETS)"
	@$(PYTHON) $(HANDLER_BUILDER) $(SEP_HANDLER_DIR) $(SEP_HANDLER_FINAL_BUILD_DIR) $(VALID_SEP_HANDLER_TARGETS)

python:
	@echo "%%%%%% copying the Python control utility"
	@cp -a $(PYTHON_DIR) $(BUILD_DIR)

clean:
	@make -C $(CTL_DIR) clean
	@make -C $(HANDLER_DIR) clean
	@make -C $(SEP_HANDLER_DIR) clean
	@rm -rf $(BUILD_DIR)/*
	@rm -rf $(PACKAGE_DIR)/*
	@echo "%%%%%% all cleaned"
