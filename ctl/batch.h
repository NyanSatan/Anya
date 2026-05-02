#ifndef BATCH_H
#define BATCH_H

#include "stddef.h"
#include "stdint.h"

int batch_init(const char *input, void **ctx);
int batch_write(void *ctx, const char *path);
void batch_quiesce(void **ctx);

int batch_get_count(void *ctx);
int batch_get_kbag(void *ctx, int idx, uint8_t *kbag);
int batch_set_key(void *ctx, int idx, uint8_t *kbag);

#endif
