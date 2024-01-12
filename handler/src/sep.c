#include <stdint.h>
#include "sep.h"
#include "aes.h"
#include "dbgwrap.h"
#include "coresight.h"

#define REQUIRE_NOERR(code, label) \
    do { \
        if (code != 0) { \
            goto label; \
        } \
    } while (0)

#define TIMEOUT 100 * 1000

#define SEP_AES_BLK			0x10
#define SEP_AES_KEY 		0x20A

#define SEP_AES_CLK_REG		(0x4)
#define SEP_AES_CTL_REG		(0x8)
#define SEP_AES_STS_REG		(0xC)

#if WITH_SKYE_SEP_AES
#define SEP_AES_IN_ADDR		(0x100)
#define SEP_AES_IV_ADDR		(0x110)
#else
#define SEP_AES_IN_ADDR		(0x40)
#define SEP_AES_IV_ADDR		(0x50)
#endif

#define SEP_AES_OUT_ADDR	(0x70)

void copy128(void *src, void *dest);

/*
 * Previously we used platform_get_raw_secure_mode() here,
 * but unfortunately it returns something different
 * than just a boolean on newer platforms such as Ellis
 */

static
uint64_t (*platform_get_cpfm)() = (void *)TARGET_PLATFORM_GET_CPFM;

bool sep_ping() {
    return (platform_get_cpfm() & 1) == 0;
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

int sep_decrypt_kbag(void *kbag, void *output) {
    uintptr_t coresight_base = TARGET_SEP_CORESIGHT_BASE;
    uintptr_t dbgwrap_base = TARGET_SEP_CORESIGHT_BASE + DBGWRAP_OFFSET;

    if (!dbgwrap_is_halted(dbgwrap_base)) {
        coresight_start(coresight_base);

        REQUIRE_NOERR(dbgwrap_halt(dbgwrap_base, TIMEOUT), fail);
    }

    /* set GID key */
    REQUIRE_NOERR(sep_write32(TARGET_SEP_AES_BASE + SEP_AES_CTL_REG, SEP_AES_KEY), fail);

    uint32_t iv[4] = { 0 };

    for (int i = 0; i < KBAG_SIZE / SEP_AES_BLK; i++) {
        /* load input portion */
        for (int x = 0; x < 4; x++) {
            REQUIRE_NOERR(
                sep_write32(
                    TARGET_SEP_AES_BASE + SEP_AES_IN_ADDR + (x * sizeof(uint32_t)),
                    *(uint32_t *)(kbag + SEP_AES_BLK * i + x * sizeof(uint32_t))
                ),
            fail);
        }

        /* load IV */
        for (int x = 0; x < 4; x++) {
            REQUIRE_NOERR(
                sep_write32(
                    TARGET_SEP_AES_BASE + SEP_AES_IV_ADDR + (x * sizeof(uint32_t)),
                    iv[x]
                ),
            fail);
        }

        /* enable clock and wait */
        REQUIRE_NOERR(sep_write32(TARGET_SEP_AES_BASE + SEP_AES_CLK_REG, 0x1), fail);

        uint32_t sts;

        do {
            REQUIRE_NOERR(sep_read32(TARGET_SEP_AES_BASE + SEP_AES_STS_REG, &sts), fail);
        } while ((sts & 1) != 1);


        /* updating IV */
        copy128(kbag + SEP_AES_BLK * i, iv);

        /* reading output */
        for (int x = 0; x < 4; x++) {
            REQUIRE_NOERR(
                sep_read32(
                    TARGET_SEP_AES_BASE + SEP_AES_OUT_ADDR + (x * sizeof(uint32_t)),
                    (output + SEP_AES_BLK * i + x * sizeof(uint32_t))
                ),
            fail);
        }
    }

    return 0;

fail:
    return -1;
}

int sep_decrypt_kbags(void *kbags, void *output, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        void *in  = kbags + (i * KBAG_SIZE);
        void *out = output + (i * KBAG_SIZE);

        if (sep_decrypt_kbag(in, out) != 0) {
            return -1;
        }
    }

    return 0;
}

