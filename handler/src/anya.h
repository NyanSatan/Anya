#ifndef ANYA_H
#define ANYA_H

#include <stdint.h>

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

/* KBAG properties */
#define KBAG_SIZE 0x30
#define KBAG_MAX_COUNT  ((TARGET_KBAG_BUFFER_SIZE - sizeof(anya_packet_hdr_t)) / KBAG_SIZE)

#endif
