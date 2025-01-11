#include "coresight.h"

static int sep_start() {
    uintptr_t coresight_base = TARGET_SEP_CORESIGHT_BASE;

    if (!coresight_is_halted(coresight_base)) {
        coresight_start(coresight_base);
        REQUIRE_NOERR(coresight_halt(coresight_base, TIMEOUT), fail);
    }

    return 0;

fail:
    return -1;
}

static int sep_read32(uint64_t address, uint32_t *value) {
    uintptr_t coresight_base = TARGET_SEP_CORESIGHT_BASE;

    /* write address to DBGDTR0 */
    REQUIRE_NOERR(coresight_write(coresight_base, address, TIMEOUT), fail);

    /* mrc p14, 0, r0, c0, c5, 0 - address to general purpose reg */
    REQUIRE_NOERR(coresight_feed_insn(coresight_base, 0xEE100E15, TIMEOUT), fail);

    /* ldc p14, c5, [r0], 4 - actual read */
    REQUIRE_NOERR(coresight_feed_insn(coresight_base, 0xECB05E01, TIMEOUT), fail);

    uint64_t value64;

    REQUIRE_NOERR(coresight_read(coresight_base, &value64, TIMEOUT), fail);

    *value = (uint32_t)value64;

    return 0;

fail:
    return -1;
}

static int sep_write32(uint64_t address, uint32_t value) {
    uintptr_t coresight_base = TARGET_SEP_CORESIGHT_BASE;

    /* write address to DBGDTR0 */
    REQUIRE_NOERR(coresight_write(coresight_base, address, TIMEOUT), fail);

    /* mrc p14, 0, r0, c0, c5, 0 - address to general purpose reg */
    REQUIRE_NOERR(coresight_feed_insn(coresight_base, 0xEE100E15, TIMEOUT), fail);

    /* write value to DBGDTR0 */
    REQUIRE_NOERR(coresight_write(coresight_base, (uint64_t)value, TIMEOUT), fail);
    
    /* stc p14, c5, [r0], 4 - actual write */
    REQUIRE_NOERR(coresight_feed_insn(coresight_base, 0xECA05E01, TIMEOUT), fail);
    
    return 0;

fail:
    return -1;
}
