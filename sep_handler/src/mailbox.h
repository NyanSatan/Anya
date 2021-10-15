#include <stdint.h>
#include <stddef.h>

#define kEndpointSEPROM 255

struct sep_message {
    uint8_t     endpoint;
    uint8_t     tag;
    uint8_t     opcode;
    uint8_t     param;
    uint32_t    data;
} __attribute__((packed));

static const
struct sep_message (*mailbox_get_message)() = (void *)TARGET_MAILBOX_GET_MESSAGE;

static const
void (*mailbox_discard)(struct sep_message *msg) = (void *)TARGET_MAILBOX_DISCARD;

static const
void (*mailbox_send_message)(
    uint8_t opcode,
    uint8_t param,
    uint32_t data
) = (void *)TARGET_MAILBOX_SEND_MESSAGE;

static
uint8_t *const mailbox_pending = (void *)TARGET_MAILBOX_PENDING;
