#include <stddef.h>

/*
 * AKF stuff
 */

#define AKF_ENDPOINT_SEP    2

static const
void (*akf_start)(
    uint8_t endpoint,
    uint64_t unk,
    uint64_t unk2
) = (void *)TARGET_AKF_START;

static const
void (*akf_stop)(uint8_t endpoint) = (void *)TARGET_AKF_STOP;


#if !TARGET_HAS_NEW_AKF_API

static const
int32_t (*akf_send_mbox)(
    uint8_t endpoint,
    const uint64_t *msg,
    uint32_t timeout
) = (void *)TARGET_AKF_SEND_MBOX;

static const
int32_t (*akf_recv_mbox)(
    uint8_t endpoint,
    uint64_t *msg,
    uint32_t timeout
) = (void *)TARGET_AKF_RECV_MBOX;

#else

static const
int32_t (*_akf_send_mbox)(
    uint8_t endpoint,
    const uint64_t *msg,
    uint32_t unk,
    uint32_t timeout
) = (void *)TARGET_AKF_SEND_MBOX;

static const
int32_t (*_akf_recv_mbox)(
    uint8_t endpoint,
    uint64_t *msg,
    uint32_t unk,
    uint32_t timeout
) = (void *)TARGET_AKF_RECV_MBOX;

#define AKF_UNKNOWN_VALUE   0x10

#define akf_send_mbox(endpoint, msg, timeout)   _akf_send_mbox(endpoint, msg, AKF_UNKNOWN_VALUE, timeout)
#define akf_recv_mbox(endpoint, msg, timeout)   _akf_recv_mbox(endpoint, msg, AKF_UNKNOWN_VALUE, timeout)

#endif

/*
 * SEP-specific stuff
 */

#define kEndpointSEPROM 255

struct sep_message {
    uint8_t     endpoint;
    uint8_t     tag;
    uint8_t     opcode;
    uint8_t     param;
    uint32_t    data;
} __attribute__((packed));
