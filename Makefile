#
# Dumpster fire of a makefile
# Someday I'll fix it, I promise
#

VALID_HANDLER_TARGETS ?= M9/B0_B1 Cyprus/B1 Cebu/B1

BUILD_DIR = build

PACKAGE_DIR = package
PACKAGE_FILE = $(PACKAGE_DIR)/Anya.zip

ASTRIS_DIR = astris
ASTRIS_TARGET = $(ASTRIS_DIR)/anya.ax

CTL_DIR = ctl
CTL_BUILD_TARGET = $(CTL_DIR)/build/anyactl

HANDLER_DIR = handler
HANDLER_BUILD_DIR = $(HANDLER_DIR)/build
HANDLER_BUILD_TARGET = $(HANDLER_BUILD_DIR)/*.bin
HANDLER_FINAL_BUILD_DIR = $(BUILD_DIR)/payloads

DIR_HELPER = mkdir -p $(BUILD_DIR) $(HANDLER_FINAL_BUILD_DIR)
PACKAGE_DIR_HELPER = mkdir -p $(PACKAGE_DIR)

.PHONY: all astris ctl handler package clean

all: astris ctl handler
	@echo "%%%%%% all done"

package: astris ctl handler
	@echo "%%%%%% packaging"
	@$(PACKAGE_DIR_HELPER)
	@rm -rf $(PACKAGE_FILE)
	@zip -x .DS_Store -r9 $(PACKAGE_FILE) $(BUILD_DIR)/*
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

handler: $(VALID_HANDLER_TARGETS)

$(VALID_HANDLER_TARGETS):
	@echo "%%%%%% building the handler payload for $@"
	@TARGET=$@ make -C $(HANDLER_DIR)
	@$(DIR_HELPER)
	@echo "%%%%%% copying the handler payload"
	@cp -a $(HANDLER_BUILD_TARGET) $(HANDLER_FINAL_BUILD_DIR)

clean:
	@make -C $(CTL_DIR) clean
	@make -C $(HANDLER_DIR) clean
	@rm -rf $(BUILD_DIR)/*
	@rm -rf $(PACKAGE_DIR)/*
	@echo "%%%%%% all cleaned"
