#include "coresight.h"
#include "time.h"

#define CORESIGHT_REG_DBGLAR        (0xFB0)
#define CORESIGHT_LOCK_ACCESS_KEY   (0xC5ACCE55)

#define CORESIGHT_REG_EDITR (0x84)

#define CORESIGHT_REG_EDSCR     (0x88)
#define CORESIGHT_EDSCR_ITE     (1 << 24)
#define CORESIGHT_EDSCR_TXFULL  (1 << 29)
#define CORESIGHT_EDSCR_RXFULL  (1 << 30)

#define CORESIGHT_REG_EDDTRRX   (0x80)
#define CORESIGHT_REG_EDDTRTX   (0x8C)

#if WITH_SEP32
    #define CORESIGHT_REG_DBGDRCR   (0x90)
    #define CORESIGHT_DBGDRCR_HALT  (1 << 0)

    #define CORESIGHT_EDSCR_HALTED  (1 << 0)
    #define CORESIGHT_EDSCR_ITREN   (1 << 13)
#endif

void coresight_start(uintptr_t cpu_base) {
    *(volatile uint32_t *)(cpu_base + CORESIGHT_REG_DBGLAR) = CORESIGHT_LOCK_ACCESS_KEY;

#if WITH_SEP32
    *(volatile uint32_t *)(cpu_base + CORESIGHT_REG_EDSCR) |= CORESIGHT_EDSCR_ITREN;
#endif
}

int coresight_feed_insn(uintptr_t cpu_base, uint32_t insn, uint64_t timeout) {
    volatile uint32_t *editr = (void *)(cpu_base + CORESIGHT_REG_EDITR);
    volatile uint32_t *edscr = (void *)(cpu_base + CORESIGHT_REG_EDSCR);

    uint64_t start = system_time();

    while ((*edscr & CORESIGHT_EDSCR_ITE) == 0) {
        if (time_has_elapsed(start, timeout)) {
            return -1;  // timeout
        }
    }

    *editr = insn;

    while ((*edscr & CORESIGHT_EDSCR_ITE) == 0) {
        if (time_has_elapsed(start, timeout)) {
            return -1;  // timeout
        }
    }

    return 0;
}

int coresight_read(uintptr_t cpu_base, uint64_t *value, uint64_t timeout) {
#if !WITH_SEP32
    volatile uint32_t *eddtrrx = (void *)(cpu_base + CORESIGHT_REG_EDDTRRX);
#endif

    volatile uint32_t *eddtrtx = (void *)(cpu_base + CORESIGHT_REG_EDDTRTX);
    volatile uint32_t *edscr   = (void *)(cpu_base + CORESIGHT_REG_EDSCR);

    uint64_t start = system_time();

    while ((*edscr & CORESIGHT_EDSCR_TXFULL) == 0) {
        if (time_has_elapsed(start, timeout)) {
            return -1;  // timeout
        }
    }

#if !WITH_SEP32
    *value = ((uint64_t)*eddtrrx << 32) | (*eddtrtx);
#else
    *value = (uint64_t)*eddtrtx;
#endif

    while ((*edscr & CORESIGHT_EDSCR_TXFULL) != 0) {
        if (time_has_elapsed(start, timeout)) {
            return -1;  // timeout
        }
    }

    return 0;
}

int coresight_write(uintptr_t cpu_base, uint64_t value, uint64_t timeout) {
    volatile uint32_t *eddtrrx = (void *)(cpu_base + CORESIGHT_REG_EDDTRRX);

#if !WITH_SEP32
    volatile uint32_t *eddtrtx = (void *)(cpu_base + CORESIGHT_REG_EDDTRTX);
#endif

    volatile uint32_t *edscr   = (void *)(cpu_base + CORESIGHT_REG_EDSCR);

    uint64_t start = system_time();

    while ((*edscr & CORESIGHT_EDSCR_RXFULL) != 0) {
        if (time_has_elapsed(start, timeout)) {
            return -1;  // timeout
        }
    }

#if !WITH_SEP32
    *eddtrtx = (value >> 32) & 0xffffffff;
    *eddtrrx = value & 0xffffffff;
#else
    *eddtrrx = (uint32_t)value;
#endif

    while ((*edscr & CORESIGHT_EDSCR_RXFULL) == 0) {
        if (time_has_elapsed(start, timeout)) {
            return -1;  // timeout
        }
    }

    return 0;
}

#if WITH_SEP32

bool coresight_is_halted(uintptr_t coresight_base) {
    volatile uint32_t *coresight_reg = (void *)(coresight_base + CORESIGHT_REG_EDSCR);
    return *coresight_reg & CORESIGHT_EDSCR_HALTED;
}

int coresight_halt(uintptr_t coresight_base, uint64_t timeout) {
    volatile uint32_t *coresight_reg = (void *)(coresight_base + CORESIGHT_REG_DBGDRCR);

    *coresight_reg |= CORESIGHT_DBGDRCR_HALT;

    uint64_t start = system_time();

    while (!coresight_is_halted(coresight_base)) {
        if (time_has_elapsed(start, timeout)) {
            return -1;  // timeout
        }
    }

    return 0;
}

#endif
