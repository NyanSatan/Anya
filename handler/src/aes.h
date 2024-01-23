#ifndef AES_H
#define AES_H

#include <stdint.h>
#include <stddef.h>
#include "firebloom.h"

#define KBAG_SIZE 0x30

enum {
    kAESEncrypt = 0x10,
    kAESDecrypt = 0x11
};

enum {
    kAESTypeGID  = 0x20000200
};

static
int (*aes_crypto_cmd)(
    uint32_t cmd,
    FIREBLOOM_PTR_DECL(void *, src),
    FIREBLOOM_PTR_DECL(void *, dst),
    size_t len,
    uint32_t opts,
    FIREBLOOM_PTR_DECL(void *, key),
    FIREBLOOM_PTR_DECL(void *, iv)
) = (void *)TARGET_AES_CRYPTO_CMD;

#endif