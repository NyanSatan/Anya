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


void coresight_start(uintptr_t cpu_base) {
    *(volatile uint32_t *)(cpu_base + CORESIGHT_REG_DBGLAR) = CORESIGHT_LOCK_ACCESS_KEY;
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
    volatile uint32_t *eddtrrx = (void *)(cpu_base + CORESIGHT_REG_EDDTRRX);
    volatile uint32_t *eddtrtx = (void *)(cpu_base + CORESIGHT_REG_EDDTRTX);
    volatile uint32_t *edscr   = (void *)(cpu_base + CORESIGHT_REG_EDSCR);

    uint64_t start = system_time();

    while ((*edscr & CORESIGHT_EDSCR_TXFULL) == 0) {
        if (time_has_elapsed(start, timeout)) {
            return -1;  // timeout
        }
    }

    *value = ((uint64_t)*eddtrrx << 32) | (*eddtrtx);

    while ((*edscr & CORESIGHT_EDSCR_TXFULL) != 0) {
        if (time_has_elapsed(start, timeout)) {
            return -1;  // timeout
        }
    }

    return 0;
}

int coresight_write(uintptr_t cpu_base, uint64_t value, uint64_t timeout) {
    volatile uint32_t *eddtrrx = (void *)(cpu_base + CORESIGHT_REG_EDDTRRX);
    volatile uint32_t *eddtrtx = (void *)(cpu_base + CORESIGHT_REG_EDDTRTX);
    volatile uint32_t *edscr   = (void *)(cpu_base + CORESIGHT_REG_EDSCR);

    uint64_t start = system_time();

    while ((*edscr & CORESIGHT_EDSCR_RXFULL) != 0) {
        if (time_has_elapsed(start, timeout)) {
            return -1;  // timeout
        }
    }

    *eddtrtx = (value >> 32) & 0xffffffff;
    *eddtrrx = value & 0xffffffff;

    while ((*edscr & CORESIGHT_EDSCR_RXFULL) == 0) {
        if (time_has_elapsed(start, timeout)) {
            return -1;  // timeout
        }
    }

    return 0;
}
