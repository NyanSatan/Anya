#ifndef MISC_H
#define MISC_H

#include <stdbool.h>

static
void (*platform_reset)(bool panic) __attribute__((noreturn)) = (void *)TARGET_PLATFORM_RESET;

#endif
