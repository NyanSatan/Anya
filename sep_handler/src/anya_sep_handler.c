/*
 * ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! !
 *
 * Remember, no .data! 
 * No preinitialized data allowed here!
 *
 * UPDATE: no .bss too! 
 *
 * ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! !
 */

#include <stdbool.h>
#include "common.h"
#include "mailbox.h"
#include "aes.h"

#define KBAG_SIZE   0x30
#define PING_VALUE  'ANYA'

enum {
    kOpCodePing = 1,
    kOpCodeReportPing = 101,

    kOpCodeSendKBAG = 2,
    kOpCodeReportSendKBAG = 102,

    kOpCodeDecryptKBAG = 3,
    kOpCodeReportDecryptKBAG = 103,

    kOpCodeGetKey = 4,
    kOpCodeReportGetKey = 104
};

static void aes_decrypt(const uint8_t *kbag, uint8_t *key) {
    uint8_t iv[AES_BLK];
    bzero(iv, AES_BLK);

    AES_CTL_REG = AES_KEY;

    for (int i = 0; i < KBAG_SIZE / AES_BLK; i++) {
        memcpy(AES_IN_ADDR, (void *)&kbag[AES_BLK * i], AES_BLK);
        memcpy(AES_IV_ADDR, iv, sizeof(iv));

        AES_CLK_REG = 0x1;
        while ((AES_STS_REG & 0x1) != 0x1) {}

        memcpy(&key[AES_BLK * i], AES_OUT_ADDR, AES_BLK);

        memcpy(iv, (void *)&kbag[AES_BLK * i], AES_BLK);
    }
}

static void msg_discard(struct sep_message *msg) {
    *mailbox_pending = false;
    mailbox_discard(msg);
    *(uint64_t *)msg = 0;
}

void __attribute__((noreturn)) anya_mailbox_loop(struct sep_message msg) {
    uint8_t kbag[KBAG_SIZE];
    uint8_t key[KBAG_SIZE];

    bzero(kbag, KBAG_SIZE);

    while (true) {
        if (*(uint64_t *)&msg == 0) {
            msg = mailbox_get_message();
        }

        if (msg.endpoint != kEndpointSEPROM) {
            msg_discard(&msg);
            continue;
        }

        *mailbox_pending = true;

        uint8_t resp_opcode = 0;
        uint8_t resp_param = 0;
        uint32_t resp_data = 0;

        switch (msg.opcode) {
            case kOpCodePing:
                resp_opcode = kOpCodeReportPing;
                resp_data = PING_VALUE;
                
                break;

            case kOpCodeSendKBAG: {
                if (msg.param >= KBAG_SIZE || msg.param % sizeof(uint32_t)) {
                    continue;
                }

                *(uint32_t *)&(kbag[msg.param]) = msg.data;

                resp_opcode = kOpCodeReportSendKBAG;
                resp_param = msg.param;

                break;
            }

            case kOpCodeDecryptKBAG:
                aes_decrypt(kbag, key);

                resp_opcode = kOpCodeReportDecryptKBAG;

                break;

            case kOpCodeGetKey: {
                if (msg.param >= KBAG_SIZE || msg.param % sizeof(uint32_t)) {
                    continue;
                }

                resp_opcode = kOpCodeReportGetKey;
                resp_param = msg.param;
                resp_data = *(uint32_t *)&(key[msg.param]);

                break;
            }

            default:
                msg_discard(&msg);
                continue;
        }

        mailbox_send_message(resp_opcode, resp_param, resp_data);


        *(uint64_t *)&msg = 0;
    }
}
