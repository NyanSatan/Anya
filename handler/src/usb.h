#include <stdint.h>

/*
 * Totally not stolen from the leaked iBoot source code
 */

#define USB_REQ_DIRECTION_MASK  0x80
#define USB_REQ_DEVICE2HOST     0x80
#define USB_REQ_HOST2DEVICE     0x00

#define EP0_IN  0x80
#define EP0_OUT 0x0

struct usb_device_request {
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} __attribute__((packed));

static
int (*handle_interface_request)(
    struct usb_device_request *request,
    uint8_t **out_buffer
) = (void *)TARGET_HANDLE_INTERFACE_REQUEST;

static
void (*usb_core_do_transfer)(
    int endpoint,
    uint8_t *buffer,
    int length,
    void (*callback)(void *)
) = (void *)TARGET_USB_CORE_DO_TRANSFER;

static
uint32_t *const total_received = (void *)TARGET_USB_TOTAL_RECEIVED;
