LIBANYA_DIR = $(SOURCE_ROOT)/libanya

LIBANYA_SOURCE = $(LIBANYA_DIR)/anya.c
LIBANYA_OBJECT = $(BUILD_ROOT)/$(LIBANYA_SOURCE:.c=.o)

LIBANYA_STATIC = $(BUILD_LIB_ROOT)/libanya.a
LIBANYA_DYNAMIC = $(BUILD_LIB_ROOT)/libanya_dynamic.dylib

LIBANYA_CFLAGS = -MMD


$(LIBANYA_STATIC): $(LIBANYA_OBJECT)
	@echo "\tassembling static libanya"
	@$(DIR_HELPER)
	@$(AR) -qc $@ $<

$(LIBANYA_OBJECT): $(LIBANYA_SOURCE)
	@echo "\tbuilding libanya"
	@$(DIR_HELPER)
	@$(CC) $(LIBANYA_CFLAGS) -c $< -o $@

-include $(LIBANYA_OBJECT:.o=.d)
