#include <stdio.h>
#include <stdlib.h>
#include "anya.h"

#define ANYA_IBOOT_FLAG (1 << 6)

#define ANYA_USB_TIMEOUT 100

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

anya_error_t anya_open(anya_device_t **dev, uint64_t ecid) {
    anya_device_t *new_dev = malloc(sizeof(anya_device_t));
    if (!new_dev) {
        return ANYA_E_OUT_OF_MEMORY;
    }

    irecv_error_t irecv_ret;
    irecv_client_t client = NULL;

    if ((irecv_ret = irecv_open_with_ecid(&client, ecid)) != IRECV_E_SUCCESS) {
        if (irecv_ret == IRECV_E_NO_DEVICE) {
            return ANYA_E_NO_DEVICE;
        }
        else {
            return ANYA_E_UNKNOWN_ERROR;
        }
    }

    anya_error_t error;

    const struct irecv_device_info *info = irecv_get_device_info(client);

    int mode;
    if ((irecv_ret = irecv_get_mode(client, &mode)) != IRECV_E_SUCCESS) {
        error = ANYA_E_UNKNOWN_ERROR;
        goto out_failure;
    }

    if (mode != IRECV_K_DFU_MODE || !(info->ibfl & ANYA_IBOOT_FLAG)) {
        error = ANYA_E_NOT_ANYA_DEVICE;
        goto out_failure;
    }

    if (irecv_usb_control_transfer(client, 0x21, ANYA_CLEAR_KBAG, 0, 0, NULL, 0, ANYA_USB_TIMEOUT) != 0) {
        error = ANYA_E_USB_ERROR;
        goto out_failure;
    }

    new_dev->conn = client;
    new_dev->cpid = info->cpid;
    new_dev->cpfm = info->cpfm;
    new_dev->ecid = info->ecid;

    *dev = new_dev;

    return ANYA_E_SUCCESS;

out_failure:
    free(new_dev);
    irecv_close(client);
    *dev = NULL;

    return error;
}

void anya_print_device(anya_device_t *dev) {
    printf("CPID:%04X, CPFM:%02X, ECID:%016llX\n", dev->cpid, dev->cpfm, dev->ecid);
}

anya_error_t anya_close(anya_device_t **dev) {
    irecv_close((*dev)->conn);
    free(*dev);
    *dev = NULL;

    return ANYA_E_SUCCESS;
}

anya_error_t anya_reboot(anya_device_t *dev) {
    irecv_usb_control_transfer(dev->conn, 0x21, ANYA_REBOOT, 0, 0, NULL, 0, ANYA_USB_TIMEOUT);
    return ANYA_E_SUCCESS;
}

anya_error_t anya_ping_sep(anya_device_t *dev, bool *result) {
    if (irecv_usb_control_transfer(dev->conn, 0xA1, ANYA_PING_SEP, 0, 0, (unsigned char *)result, sizeof(bool), ANYA_USB_TIMEOUT) != sizeof(bool)) {
        return ANYA_E_USB_ERROR;
    }

    return ANYA_E_SUCCESS;
}

anya_error_t anya_decrypt_internal(anya_device_t *dev, uint8_t kbag[], uint8_t key[], bool sep) {
    if (irecv_usb_control_transfer(dev->conn, 0x21, DFU_DNLOAD, 0, 0, kbag, KBAG_SIZE, ANYA_USB_TIMEOUT) != KBAG_SIZE) {
        return ANYA_E_USB_ERROR;
    }

    if (irecv_usb_control_transfer(dev->conn, 0xA1, sep ? ANYA_DECRYPT_SEP_KBAG : ANYA_DECRYPT_KBAG, 0, 0, key, KBAG_SIZE, ANYA_USB_TIMEOUT) != KBAG_SIZE) {
        return ANYA_E_USB_ERROR;
    }

    return ANYA_E_SUCCESS;
}

const char* anya_strerror(anya_error_t error) {
    switch (error) {
        case ANYA_E_SUCCESS:
            return "success";
        case ANYA_E_OUT_OF_MEMORY:
            return "out of memory";
        case ANYA_E_NO_DEVICE:
            return "no device";
        case ANYA_E_NOT_ANYA_DEVICE:
            return "not Anya device";
        case ANYA_E_USB_ERROR:
            return "USB error";
        case ANYA_E_UNKNOWN_ERROR:
            return "unknown error";
        default:
            return "undefined error";
    }
}
