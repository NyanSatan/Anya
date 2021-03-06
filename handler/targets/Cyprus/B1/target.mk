TARGET_NAME = Cyprus-B1
TARGET_CAN_HAVE_SEP_SUPPORT = true

TARGET_TRAMPOLINE 	= 0x19C018000
TARGET_LOADADDR 	= 0x19C030000

TARGET_HANDLE_INTERFACE_REQUEST = 0x10000E3EC
TARGET_USB_CORE_DO_TRANSFER 	= 0x10000E0D8
TARGET_USB_TOTAL_RECEIVED       = 0x19C010BEC
TARGET_AES_CRYPTO_CMD			= 0x100009BE8
TARGET_PLATFORM_RESET			= 0x100006ED0

#
# SEP support
#

TARGET_HAS_NEW_AKF_API 	= 0
TARGET_AKF_START		= 0x100001D14
TARGET_AKF_STOP			= 0x100001F84
TARGET_AKF_SEND_MBOX	= 0x100001F6C
TARGET_AKF_RECV_MBOX	= 0x100001F3C
