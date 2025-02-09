#
# This is disabled for now,
# since I have got no hardware that can boot into iBoot
#

TARGET_NAME = Coll-B0

TARGET_TRAMPOLINE 	= 0x10007C000
TARGET_LOADADDR 	= 0x1FC07C000

TARGET_HAS_FIREBLOOM = true

TARGET_HANDLE_INTERFACE_REQUEST = 0x100020144
TARGET_USB_CORE_DO_TRANSFER 	= 0x10001F8C8
TARGET_USB_TOTAL_RECEIVED       = 0x1FC03CDB0
TARGET_AES_CRYPTO_CMD			= 0x10000E0AC
TARGET_PLATFORM_RESET			= 0x100009904
TARGET_FIREBLOOM_KBAG_TYPE		= 0x10004A240
TARGET_FIREBLOOM_BYTE_TYPE		= 0x10004C840

#
# SEP support
#

TARGET_CAN_HAVE_SEP_SUPPORT = true

TARGET_SEP_CORESIGHT_BASE			= 0x2A2010000
TARGET_SEP_AES_BASE					= 0x2A0B00000
TARGET_PLATFORM_GET_CPFM			= 0x10003329C
TARGET_SYSTEM_TIME					= 0x100034AA0
