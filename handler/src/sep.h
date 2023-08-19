#ifndef SEP_H
#define SEP_H

#include <stdbool.h>
#include <stdint.h>

bool sep_ping();
int  sep_decrypt_kbag(void *kbag, void *output);
int  sep_decrypt_kbags(void *kbags, void *output, uint32_t count);

#endif
