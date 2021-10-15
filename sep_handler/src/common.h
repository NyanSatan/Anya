#include <stddef.h>

static const
void (*memcpy)(
    void *dest,
    void *src,
    size_t len
) = (void *)TARGET_MEMCPY;

static const
void (*bzero)(void *s, size_t n) = (void *)TARGET_BZERO;;
