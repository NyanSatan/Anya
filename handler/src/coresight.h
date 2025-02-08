#ifndef CORESIGHT_H
#define CORESIGHT_H

#include <stdint.h>
#include <stdbool.h>

void coresight_start(uintptr_t cpu_base);

int coresight_feed_insn(uintptr_t cpu_base, uint32_t insn, uint64_t timeout);
int coresight_read(uintptr_t cpu_base, uint64_t *value, uint64_t timeout);
int coresight_write(uintptr_t cpu_base, uint64_t value, uint64_t timeout);

#if WITH_SEP32

bool coresight_is_halted(uintptr_t cpu_base);
int  coresight_halt(uintptr_t cpu_base, uint64_t timeout);

#endif

#endif
