#include "anya.h"
#include "ap.h"
#include "aes.h"

#if WITH_DPA_HACK
    uint8_t dummy_kbag[KBAG_SIZE];
    bool ap_decrypted = false;
#endif

#define PADDING_SIZE 0x10

uint8_t kbag_buffer[KBAG_MAX_COUNT * (KBAG_SIZE + PADDING_SIZE)];

/* operations for AP decrypt */
size_t copyin(void *in, void *out, size_t count);
size_t copyout(void *in, void *out, size_t count);

int ap_decrypt_kbags(void *kbags, void *output, uint32_t count) {
    /* copying KBAGs to our buffer */
    size_t to_decrypt_size = copyin(kbags, kbag_buffer, count);

    /* decrypting! */
    if (aes_crypto_cmd(
            kAESDecrypt,
            FIREBLOOM_PTR(kbag_buffer, kbag_buffer, kbag_buffer + to_decrypt_size, TARGET_FIREBLOOM_KBAG_TYPE),
            FIREBLOOM_PTR(kbag_buffer, kbag_buffer, kbag_buffer + to_decrypt_size, TARGET_FIREBLOOM_KBAG_TYPE),
            to_decrypt_size,
            kAESTypeGID,
            FIREBLOOM_NULL_PTR,
            FIREBLOOM_NULL_PTR
        ) != 0) {
        return -1;
    }

#if WITH_DPA_HACK
    ap_decrypted = true;
#endif

    /* copying KBAGs back */
    copyout(kbag_buffer, output, count);

    return 0;
}

