#include "dbgwrap.h"
#include "coresight.h"

static int sep_start() {
    uintptr_t coresight_base = TARGET_SEP_CORESIGHT_BASE;
    uintptr_t dbgwrap_base = TARGET_SEP_CORESIGHT_BASE + DBGWRAP_OFFSET;

    if (!dbgwrap_is_halted(dbgwrap_base)) {
        coresight_start(coresight_base);
        REQUIRE_NOERR(dbgwrap_halt(dbgwrap_base, TIMEOUT), fail);
    }

    return 0;

fail:
    return -1;
}

static int sep_read32(uint64_t address, uint32_t *value) {
    uintptr_t coresight_base = TARGET_SEP_CORESIGHT_BASE;

    /* write address to DBGDTR0 */
    REQUIRE_NOERR(coresight_write(coresight_base, address, TIMEOUT), fail);

    /* mrs x0, DBGDTR0 - address to general purpose reg */
    REQUIRE_NOERR(coresight_feed_insn(coresight_base, 0xD5330400, TIMEOUT), fail);

    /* ldr w1, [x0], 4 - actual read */
    REQUIRE_NOERR(coresight_feed_insn(coresight_base, 0xB8404401, TIMEOUT), fail);

    /* msr DBGDTR0, x1 - set the read value to DBGDTR0 */
    REQUIRE_NOERR(coresight_feed_insn(coresight_base, 0xD5130401, TIMEOUT), fail);

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

    /* mrs x0, DBGDTR0 - address to general purpose reg */
    REQUIRE_NOERR(coresight_feed_insn(coresight_base, 0xD5330400, TIMEOUT), fail);

    /* write value to DBGDTR0 */
    REQUIRE_NOERR(coresight_write(coresight_base, (uint64_t)value, TIMEOUT), fail);

    /* mrs x1, DBGDTR0 - value to general purpose reg */
    REQUIRE_NOERR(coresight_feed_insn(coresight_base, 0xD5330401, TIMEOUT), fail);
    
    /* str w1, [x0], 4 - actual write */
    REQUIRE_NOERR(coresight_feed_insn(coresight_base, 0xB8004401, TIMEOUT), fail);
    
    return 0;

fail:
    return -1;
}
