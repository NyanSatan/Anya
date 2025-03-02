#ifndef AP_H
#define AP_H

#include <stdint.h>
#include <stdbool.h>

#if WITH_DPA_HACK
    extern uint8_t dummy_kbag[KBAG_SIZE];
    extern bool ap_decrypted;
#endif

int ap_decrypt_kbags(void *kbags, void *output, uint32_t count);

#endif
