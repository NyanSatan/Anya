/*
 * ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! !
 *
 * Remember, no .data! 
 * No preinitialized data allowed here!
 *
 * UPDATE: no .bss too! 
 *
 * ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! !
 */

#include <stdbool.h>
#include "usb.h"
#include "common.h"

enum {
    DFU_DETACH = 0,
    DFU_DNLOAD,
    DFU_UPLOAD,
    DFU_GETSTATUS,
    DFU_CLR_STATUS,
    DFU_GETSTATE,
    DFU_ABORT,
    ANYA_DECRYPT_KBAG,
    ANYA_CLEAR_KBAG,
    ANYA_REBOOT
};

#define KBAG_SIZE 0x30

static void reset_counter() {
    *total_received = 0;
}

int anya_handle_interface_request(struct usb_device_request *request, uint8_t **out_buffer) {
    uint8_t  bmRequestType = request->bmRequestType;
    uint8_t  bRequest      = request->bRequest;
    uint16_t wLength       = request->wLength;

    if ((bmRequestType & USB_REQ_DIRECTION_MASK) == USB_REQ_HOST2DEVICE) {
        switch (bRequest) {
            case DFU_DETACH:
                return handle_interface_request(request, out_buffer);

            case DFU_DNLOAD: {
                if (*total_received + wLength > KBAG_SIZE) {
                    return -1;
                }

                return handle_interface_request(request, out_buffer);
            }

            case DFU_CLR_STATUS:
            case DFU_ABORT:
                //disallow DFU_ABORT/DFU_CLR_STATUS,
                //as they will make it re-enter DFU
                return -1;

            case ANYA_CLEAR_KBAG:
                //not really clear, but rather just reset the counter
                reset_counter();
                return 0;

            case ANYA_REBOOT:
                platform_reset();
        }

    } else if ((bmRequestType & USB_REQ_DIRECTION_MASK) == USB_REQ_DEVICE2HOST) {
        switch (bRequest) {
            case DFU_UPLOAD:
            case DFU_GETSTATUS:
            case DFU_GETSTATE:
                return handle_interface_request(request, out_buffer);

            case ANYA_DECRYPT_KBAG: {
                if (*total_received != KBAG_SIZE) {
                    return -1;
                }

                if (aes_crypto_cmd(
                    kAESDecrypt,
                    (void *)TARGET_LOADADDR,
                    (void *)TARGET_LOADADDR,
                    KBAG_SIZE,
                    kAESTypeGID,
                    NULL,
                    NULL) != 0)
                {
                    return -1;
                }

                usb_core_do_transfer(EP0_IN, (uint8_t *)TARGET_LOADADDR, KBAG_SIZE, NULL);

                reset_counter();
                return 0;
            }
        }
    }

    return -1;
}
