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

#if WITH_SEP
#include "sep.h"
#endif


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
    ANYA_REBOOT,
    ANYA_PING_SEP,
    ANYA_DECRYPT_SEP_KBAG
};

/*
 * TODO: move these to a separate file
 */

#if WITH_SEP

#define SEP_TIMEOUT (2 * 1000 * 1000)
#define PING_VALUE  'ANYA'

enum {
    kOpCodePing = 1,
    kOpCodeReportPing = 101,

    kOpCodeSendKBAG = 2,
    kOpCodeReportSendKBAG = 102,

    kOpCodeDecryptKBAG = 3,
    kOpCodeReportDecryptKBAG = 103,

    kOpCodeGetKey = 4,
    kOpCodeReportGetKey = 104
};

static struct sep_message sep_create_message(uint8_t opcode, uint8_t param, uint32_t data) {
    struct sep_message msg;

    msg.endpoint = kEndpointSEPROM;
    msg.tag = 0;
    msg.opcode = opcode;
    msg.param = param;
    msg.data = data;

    return msg;
}

static int sep_send_message(struct sep_message *dest, uint8_t opcode, uint8_t param, uint32_t data) {
    struct sep_message send = sep_create_message(opcode, param, data);

    if (akf_send_mbox(AKF_ENDPOINT_SEP, (uint64_t *)&send, SEP_TIMEOUT) != 0) {
        return -1;
    }

    return akf_recv_mbox(AKF_ENDPOINT_SEP, (uint64_t *)dest, SEP_TIMEOUT);
}

static void sep_start() {
    akf_start(AKF_ENDPOINT_SEP, 0, 0);
}

static void sep_stop() {
    akf_stop(AKF_ENDPOINT_SEP);
}

static bool sep_ping() {
    sep_start();

    struct sep_message rcvd;

    int ret = sep_send_message(&rcvd, kOpCodePing, 0, 0);

    sep_stop();

    return (bool)((ret == 0) && (rcvd.opcode == kOpCodeReportPing) && (rcvd.data == PING_VALUE));
}

static int sep_decrypt(const void *kbag, void *key) {
    sep_start();

    int ret = 0;

    struct sep_message rcvd;

    /*
     * Sending KBAG
     */

    for (int i = 0; i < KBAG_SIZE; i += 4) {
        if (!(sep_send_message(&rcvd, kOpCodeSendKBAG, i, *(uint32_t *)(kbag + i)) == 0 &&
            rcvd.opcode == kOpCodeReportSendKBAG && 
            rcvd.param == i)
            ) {
            goto fail;
        }
    }

    /*
     * Decrypting KBAG
     */

    if (!(sep_send_message(&rcvd, kOpCodeDecryptKBAG, 0, 0) == 0 && rcvd.opcode == kOpCodeReportDecryptKBAG)) {
        goto fail;
    }

    /*
     * Retrieving key
     */

    for (int i = 0; i < KBAG_SIZE; i += 4) {
        if (!(sep_send_message(&rcvd, kOpCodeGetKey, i, 0) == 0 &&
            rcvd.opcode == kOpCodeReportGetKey && 
            rcvd.param == i)
            ) {
            goto fail;
        }

        *(uint32_t *)(key + i) = rcvd.data;
    }

    goto out;

fail:
    ret = -1;

out:
    sep_stop();

    return ret;
}

#endif

static int ap_decrypt(const void *kbag, void *key) {
    return aes_crypto_cmd(kAESDecrypt, (void *)kbag, key, KBAG_SIZE, kAESTypeGID, NULL, NULL);
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

            case ANYA_DECRYPT_KBAG:
            case ANYA_DECRYPT_SEP_KBAG: {
                if (*total_received != KBAG_SIZE) {
                    return -1;
                }

                /*
                 * TODO: there should be a more beautiful way to write this
                 */

#if WITH_SEP
                if (bRequest == ANYA_DECRYPT_SEP_KBAG) {
                    if (sep_decrypt((void *)TARGET_LOADADDR, (void *)TARGET_LOADADDR) != 0) {
                        return -1;
                    }
                } else {
                    if (ap_decrypt((void *)TARGET_LOADADDR, (void *)TARGET_LOADADDR) != 0) {
                        return -1;
                    }
                }
#else
                if (bRequest == ANYA_DECRYPT_SEP_KBAG) {
                    return -1;
                } else {
                    if (ap_decrypt((void *)TARGET_LOADADDR, (void *)TARGET_LOADADDR) != 0) {
                        return -1;
                    }
                }
#endif

                usb_core_do_transfer(EP0_IN, (uint8_t *)TARGET_LOADADDR, KBAG_SIZE, NULL);

                reset_counter();
                return 0;
            }

            case ANYA_PING_SEP: {
#if WITH_SEP
                bool ack = sep_ping();
#else
                bool ack = false;
#endif

                /*
                 * TODO: this is ugly,
                 * doing this to ensure KBAG won't be touched
                 * in case someone sends the ping request
                 * between sending a KBAG and decrypting it
                 */

                *(bool *)(TARGET_LOADADDR + KBAG_SIZE) = ack;

                usb_core_do_transfer(EP0_IN, (uint8_t *)(TARGET_LOADADDR + KBAG_SIZE), sizeof(bool), NULL);

                return 0;
            }
        }
    }

    return -1;
}
