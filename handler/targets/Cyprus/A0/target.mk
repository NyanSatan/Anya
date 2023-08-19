TARGET_NAME = Cyprus-A0

TARGET_TRAMPOLINE 	= 0x19C1C8000
TARGET_LOADADDR 	= 0x19C000000

TARGET_HANDLE_INTERFACE_REQUEST = 0x10000E064
TARGET_USB_CORE_DO_TRANSFER 	= 0x10000DD50
TARGET_USB_TOTAL_RECEIVED       = 0x19C1C0BEC
TARGET_AES_CRYPTO_CMD			= 0x100009870
TARGET_PLATFORM_RESET			= 0x100006CCC

#
# SEP support
#

TARGET_CAN_HAVE_SEP_SUPPORT = true

TARGET_SEP_CORESIGHT_BASE			= 0x242010000
TARGET_SEP_AES_BASE					= 0x241040000
TARGET_PLATFORM_GET_RAW_SECURE_MODE = 0x100011B38
TARGET_SYSTEM_TIME					= 0x1000097EC
