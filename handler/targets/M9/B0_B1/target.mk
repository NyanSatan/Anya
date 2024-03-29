TARGET_NAME = M9-B0_B1

TARGET_TRAMPOLINE 	= 0x1801C8000
TARGET_LOADADDR 	= 0x180000000

TARGET_HANDLE_INTERFACE_REQUEST = 0x10000E388
TARGET_USB_CORE_DO_TRANSFER 	= 0x10000E074
TARGET_USB_TOTAL_RECEIVED       = 0x1801C037C
TARGET_AES_CRYPTO_CMD			= 0x100009B44
TARGET_PLATFORM_RESET			= 0x100007170

#
# SEP support
#

TARGET_CAN_HAVE_SEP_SUPPORT = true

TARGET_SEP_CORESIGHT_BASE			= 0x242010000
TARGET_SEP_AES_BASE					= 0x241040000
TARGET_PLATFORM_GET_CPFM			= 0x10000A83C
TARGET_SYSTEM_TIME					= 0x100009AC0
