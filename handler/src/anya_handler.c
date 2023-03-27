/*
 * ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! !
 *
 * Remember, no .data! 
 * No preinitialized data allowed here!
 *
 * ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! !
 */

#include <stdbool.h>
#include "usb.h"
#include "common.h"

#define KBAG_SIZE 0x30

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

typedef struct __attribute__((packed)) {
#define ANYA_MAGIC  'ANYA'
    uint32_t magic;
    uint32_t kbag_count;
#define AnyaPacketFlagDecrypted (1 << 0)
    uint32_t flags;
    uint32_t reserved;
} anya_packet_hdr_t;

static int ap_decrypt(void *kbag, void *key) {
    return aes_crypto_cmd(kAESDecrypt, kbag, key, KBAG_SIZE, kAESTypeGID, NULL, NULL);
}

static int anya_packet_decrypt() {
    /* must contain at least one KBAG */
    if (*total_received < sizeof(anya_packet_hdr_t) + KBAG_SIZE) {
        return -1;
    }

    anya_packet_hdr_t *packet = (void *)TARGET_LOADADDR;

    /* checking magic */
    if (packet->magic != ANYA_MAGIC) {
        return -1;
    }

    /* checking if it's already decrypted */
    if (packet->flags & AnyaPacketFlagDecrypted) {
        return -1;
    }

    /* checking if we received enough data for requested KBAG count */
    if (packet->kbag_count * KBAG_SIZE > *total_received - sizeof(anya_packet_hdr_t)) {
        return -1;
    }

    /* decrypting! */
    uint8_t *kbags = (void *)TARGET_LOADADDR + sizeof(anya_packet_hdr_t);

    for (uint32_t i = 0; i < packet->kbag_count; i++) {
        uint8_t *current_kbag = kbags + i * KBAG_SIZE;

        if (ap_decrypt(current_kbag, current_kbag) != 0) {
            return -1;
        }
    }

    /* setting decrypted flag for host */
    packet->flags |= AnyaPacketFlagDecrypted;

    return 0;
}

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
                if (*total_received + wLength > TARGET_KBAG_BUFFER_SIZE) {
                    return -1;
                }

                return handle_interface_request(request, out_buffer);
            }

            case DFU_CLR_STATUS:
            case DFU_ABORT:
                // disallow DFU_ABORT/DFU_CLR_STATUS,
                // as they will make it re-enter DFU
                return -1;

            case ANYA_CLEAR_KBAG:
                // not really clear, but rather just reset the counter
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
                if (anya_packet_decrypt() != 0) {
                    return -1;
                }

                usb_core_do_transfer(EP0_IN, (uint8_t *)TARGET_LOADADDR, *total_received, NULL);

                reset_counter();
                return 0;
            }
        }
    }

    return -1;
}
