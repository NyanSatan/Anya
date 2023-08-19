#ifndef TIME_H
#define TIME_H

#include <stdint.h>
#include <stdbool.h>

static
uint64_t (*system_time)() = (void *)TARGET_SYSTEM_TIME;

bool time_has_elapsed(uint64_t start, uint64_t timeout);

#endif
