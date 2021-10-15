#include <stddef.h>

#define REG32(addr)	*(volatile uint32_t*)(addr)
#define ADDR(addr)	(void *)addr

#define AES_BLK			0x10
#define AES_KEY 		0x20A

#define AES_CLK_REG		REG32(TARGET_AES_BASE + 0x4)
#define AES_CTL_REG		REG32(TARGET_AES_BASE + 0x8)
#define AES_STS_REG		REG32(TARGET_AES_BASE + 0xC)
#define AES_IN_ADDR		ADDR(TARGET_AES_BASE + 0x40)
#define AES_IV_ADDR		ADDR(TARGET_AES_BASE + 0x50)
#define AES_OUT_ADDR	ADDR(TARGET_AES_BASE + 0x70)
