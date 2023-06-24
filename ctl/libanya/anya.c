#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "anya.h"

#define ANYA_IBOOT_FLAG (1 << 6)

#define ANYA_USB_TIMEOUT 100

#define DFU_MAX_PACKET_SIZE     0x800
#define ANYA_MAX_PACKET_SIZE    0x8000

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

struct anya_device {
    irecv_client_t  conn;
    uint16_t        cpid;
    uint8_t         cpfm;
    uint64_t        ecid;
    void           *io_buffer;
};

typedef struct __attribute__((packed)) {
#define ANYA_MAGIC  'ANYA'
    uint32_t magic;
    uint32_t kbag_count;
#define AnyaPacketFlagDecrypted (1 << 0)
    uint32_t flags;
    uint32_t reserved;
} anya_packet_hdr_t;

#define KBAG_MAX_COUNT  ((ANYA_MAX_PACKET_SIZE - sizeof(anya_packet_hdr_t)) / KBAG_SIZE)
#define MIN(x, y)   (x < y ? x : y)

anya_error_t anya_open(anya_device_t **dev, uint64_t ecid) {
    irecv_error_t irecv_ret;
    anya_error_t error = ANYA_E_SUCCESS;
    irecv_client_t client = NULL;
    anya_device_t *new_dev = NULL;
    void *io_buffer = NULL;

    if ((irecv_ret = irecv_open_with_ecid(&client, ecid)) != IRECV_E_SUCCESS) {
        if (irecv_ret == IRECV_E_NO_DEVICE) {
            return ANYA_E_NO_DEVICE;
        } else {
            return ANYA_E_UNKNOWN_ERROR;
        }
    }

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

    new_dev = malloc(sizeof(anya_device_t));
    if (!new_dev) {
        error = ANYA_E_OUT_OF_MEMORY;
        goto out_failure;
    }

    io_buffer = malloc(ANYA_MAX_PACKET_SIZE);
    if (!io_buffer) {
        error = ANYA_E_OUT_OF_MEMORY;
        goto out_failure;
    }

    new_dev->conn = client;
    new_dev->cpid = info->cpid;
    new_dev->cpfm = info->cpfm;
    new_dev->ecid = info->ecid;
    new_dev->io_buffer = io_buffer;

    *dev = new_dev;

    return ANYA_E_SUCCESS;

out_failure:
    free(new_dev);
    free(io_buffer);
    irecv_close(client);

    *dev = NULL;

    return error;
}

void anya_print_device(anya_device_t *dev) {
    printf("CPID:%04X CPFM:%02X ECID:%016llX\n", dev->cpid, dev->cpfm, dev->ecid);
}

anya_error_t anya_close(anya_device_t **dev) {
    irecv_close((*dev)->conn);
    free((*dev)->io_buffer);
    free(*dev);

    *dev = NULL;

    return ANYA_E_SUCCESS;
}

static size_t dfu_send_data(anya_device_t *dev, uint8_t *data, size_t size) {
    size_t index = 0;

    while (index != size) {
        size_t amount = MIN(DFU_MAX_PACKET_SIZE, size - index);

        if (irecv_usb_control_transfer(dev->conn, 0x21, DFU_DNLOAD, 0, 0, data + index, amount, ANYA_USB_TIMEOUT) != amount) {
            return -1;
        }

        index += amount;
    }
    
    return index;
}

anya_error_t anya_decrypt(anya_device_t *dev, uint8_t kbags[], uint8_t keys[], size_t count) {
    off_t offset = 0;

    while (count != 0) {
        size_t curr_count = MIN(count, KBAG_MAX_COUNT);
        uint8_t *curr_kbags = kbags + offset * KBAG_SIZE;
        uint8_t *curr_keys = keys + offset * KBAG_SIZE;

        anya_packet_hdr_t *packet = dev->io_buffer;

        packet->magic = ANYA_MAGIC;
        packet->kbag_count = curr_count;
        packet->flags = 0;
        packet->reserved = 0;

        memcpy(packet + 1, curr_kbags, curr_count * KBAG_SIZE);

        size_t packet_size = sizeof(anya_packet_hdr_t) + curr_count * KBAG_SIZE;

        if (dfu_send_data(dev, dev->io_buffer, packet_size) != packet_size) {
            return ANYA_E_USB_ERROR;
        }

        if (irecv_usb_control_transfer(dev->conn, 0xA1, ANYA_DECRYPT_KBAG, 0, 0, dev->io_buffer, packet_size, ANYA_USB_TIMEOUT) != packet_size) {
            return ANYA_E_USB_ERROR;
        }

        if (packet->magic != ANYA_MAGIC) {
            return ANYA_E_HANDLER_ERROR;
        }

        if (packet->kbag_count != curr_count) {
            return ANYA_E_HANDLER_ERROR;
        }

        if (!(packet->flags & AnyaPacketFlagDecrypted)) {
            return ANYA_E_HANDLER_ERROR;
        }

        memcpy(curr_keys, packet + 1, curr_count * KBAG_SIZE);

        offset += curr_count;
        count -= curr_count;
    }

    return ANYA_E_SUCCESS;
}

anya_error_t anya_reboot(anya_device_t *dev) {
    irecv_usb_control_transfer(dev->conn, 0x21, ANYA_REBOOT, 0, 0, NULL, 0, ANYA_USB_TIMEOUT);
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
        case ANYA_E_HANDLER_ERROR:
            return "invalid response from handler";
        case ANYA_E_UNKNOWN_ERROR:
            return "unknown error";
        default:
            return "undefined error";
    }
}
