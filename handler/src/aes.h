#ifndef AES_H
#define AES_H

#include <stdint.h>
#include <stddef.h>

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
    void *src,
    void *dst,
    size_t len,
    uint32_t opts,
    const void *key,
    void *iv
) = (void *)TARGET_AES_CRYPTO_CMD;

#endif