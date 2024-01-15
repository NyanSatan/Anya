/*
 * ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! !
 *
 * Remember, no data! 
 * No preinitialized data allowed here!
 *
 * ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! !
 */

#include <stdbool.h>
#include <stddef.h>
#include "usb.h"
#include "aes.h"
#include "misc.h"

/* build tag string */
const char __attribute__((used, aligned(16))) BUILD_STRING[] = TARGET_BUILD_STRING;

/* DFU bRequest selectors */
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
    ANYA_REBOOT,
    ANYA_PING_SEP
};

/* Anya packet header definition */
typedef struct __attribute__((packed)) {
#define ANYA_MAGIC  'ANYA'
    uint32_t magic;
    uint32_t kbag_count;
#define AnyaPacketFlagDecrypted (1 << 0)
#define AnyaPacketFlagSEP       (1 << 1)
    uint32_t flags;
    uint32_t reserved;
} anya_packet_hdr_t;


/* SEP stuff */
#if WITH_SEP
#include "sep.h"

typedef enum {
    SEP_UNKNOWN = 0,
    SEP_UNSUPPORTED,
    SEP_SUPPORTED
} sep_status_t;

sep_status_t sep_status = SEP_UNKNOWN;

#if WITH_DPA_HACK
uint8_t dummy_kbag[KBAG_SIZE];
bool ap_decrypted = false;
#endif

#endif

/*
 * Older platforms (definetely Skye and Cyprus) do not like
 * unaligned adresses passed to usb_core_do_transfer()
 *
 * The proper way to fix it is using out_buffer provided
 * by SecureROM to handle_interface_request() which is
 * for sure aligned
 */

bool __attribute__((aligned(4))) sep_status_dfu = false;

/* KBAG properties */
#define PADDING_SIZE 0x10

#define KBAG_MAX_COUNT  ((TARGET_KBAG_BUFFER_SIZE - sizeof(anya_packet_hdr_t)) / KBAG_SIZE)

uint8_t kbag_buffer[KBAG_MAX_COUNT * (KBAG_SIZE + PADDING_SIZE)];

/* operations for AP decrypt */
size_t copyin(void *in, void *out, size_t count);
size_t copyout(void *in, void *out, size_t count);

static int ap_decrypt_kbags(void *kbags, void *output, uint32_t count) {
    /* copying KBAGs to our buffer */
    size_t to_decrypt_size = copyin(kbags, kbag_buffer, count);

    /* decrypting! */
    if (aes_crypto_cmd(kAESDecrypt, kbag_buffer, kbag_buffer, to_decrypt_size, kAESTypeGID, NULL, NULL) != 0) {
        return -1;
    }

#if WITH_DPA_HACK
    ap_decrypted = true;
#endif

    /* copying KBAGs back */
    copyout(kbag_buffer, output, count);

    return 0;
}


/* decrypt function - parses packet header and calls decrypt functions */
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

    /* checking if we are trying to decrypt SEP KBAGs*/
    if (packet->flags & AnyaPacketFlagSEP) {
#if WITH_SEP       
        if (sep_status == SEP_UNSUPPORTED) {
            return -1;
        } else if (sep_status == SEP_UNKNOWN) {
            sep_status = sep_ping() ? SEP_SUPPORTED : SEP_UNSUPPORTED;

            if (sep_status == SEP_UNSUPPORTED) {
                return -1;
            }
        }
#else
        return -1;
#endif
    }

    /* checking if we received enough data for requested KBAG count */
    if (packet->kbag_count * KBAG_SIZE > *total_received - sizeof(anya_packet_hdr_t)) {
        return -1;
    }

    /* decrypting! */
    uint8_t *kbags = (void *)TARGET_LOADADDR + sizeof(anya_packet_hdr_t);

    int ret;

#if WITH_SEP
    if (packet->flags & AnyaPacketFlagSEP) {

#if WITH_DPA_HACK
        if (!ap_decrypted) {
            if (ap_decrypt_kbags(dummy_kbag, dummy_kbag, 1) != 0) {
                return -1;
            }
        }
#endif

        ret = sep_decrypt_kbags(kbags, kbags, packet->kbag_count);
    }
    else
#endif
    {
        ret = ap_decrypt_kbags(kbags, kbags, packet->kbag_count);
    }

    if (ret != 0) {
        return -1;
    }

    /* setting decrypted flag for host */
    packet->flags |= AnyaPacketFlagDecrypted;

    return 0;
}

/* resets DFU counter - on both failure, success or user request */
static void reset_counter() {
    *total_received = 0;
}

/* the main function - handles USB requests */
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
                // as they will make us re-enter DFU
                return -1;

            case ANYA_CLEAR_KBAG:
                // not really clear, but rather just reset the counter
                reset_counter();
                return 0;

            case ANYA_REBOOT:
                platform_reset(false);
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

            case ANYA_PING_SEP: {
#if WITH_SEP
                sep_status_dfu = sep_ping();
#else
                sep_status_dfu = false;
#endif
                usb_core_do_transfer(EP0_IN, (uint8_t *)&sep_status_dfu, sizeof(sep_status_dfu), NULL);

                return 0;
            }
        }
    }

    return -1;
}
