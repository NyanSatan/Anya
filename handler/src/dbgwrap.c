#include "dbgwrap.h"
#include "time.h"

#define DBGWRAP_HALT    (1 << 31)
#define DBGWRAP_HALTED  (1 << 28)

bool dbgwrap_is_halted(uintptr_t dbgwrap_base) {
    volatile uint64_t *dbgwrap_reg = (void *)(dbgwrap_base);
    return *dbgwrap_reg & DBGWRAP_HALTED;
}

int dbgwrap_halt(uintptr_t dbgwrap_base, uint64_t timeout) {
    volatile uint64_t *dbgwrap_reg = (void *)(dbgwrap_base);

    *dbgwrap_reg |= DBGWRAP_HALT;

    uint64_t start = system_time();

    while (!dbgwrap_is_halted(dbgwrap_base)) {
        if (time_has_elapsed(start, timeout)) {
            return -1;  // timeout
        }
    }

    return 0;
}
