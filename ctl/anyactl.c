#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include "libanya/anya.h"
#include "libanya/log.h"

/*
 * These 2 are stolen from @xerub
 */

static long str2hex(unsigned long long max, unsigned char *buf, const char *str) {
    unsigned char *ptr = buf;
    int seq = -1;
    while (max > 0) {
        int nibble = *str++;

        if (nibble >= '0' && nibble <= '9') {
            nibble -= '0';
        } else {
            nibble |= 0x20;
            if (nibble >= 'a' && nibble <= 'f') {
                nibble -= 'a' - 10;
            } else {
                break;
            }
        }

        if (seq >= 0) {
            *buf++ = (seq << 4) | nibble;
            max--;
            seq = -1;
        } else {
            seq = nibble;
        }
    }

    return buf - ptr;
}

static void hex2str(char *str, int buflen, const unsigned char *buf) {
    static const char h2a[] = "0123456789ABCDEF";

    for (; buflen > 0; --buflen) {
        unsigned char byte = *buf++;
        *str++ = h2a[byte >> 4];
        *str++ = h2a[byte & 0xF];
    }

    *str = '\0';
}

/*
 * The rest are mine
 */

static int open_device(anya_device_t **dev, uint64_t ecid) {
    anya_error_t error;

    if ((error = anya_open(dev, ecid)) != ANYA_E_SUCCESS) {
        ANYA_ERROR("failed to connect - %s", anya_strerror(error));
        return -1;
    }

    anya_print_device(*dev);

    return 0;
}

static void usage(const char *program_name) {
    printf("usage: %s ARG[s]\n", program_name);
    printf("\n");
    printf("where ARG[s] must be one of the following:\n");
    printf("\t-k KBAG\tspecifies KBAG to be decrypted\n");
    printf("\t-b NUM\truns benchmark with NUM random KBAGs\n");
    printf("\n");
    printf("you can also use these one with both of the above:\n");
    printf("\t-s\tuses SEP GID (if possible)\n");
    printf("\t-e ECID\t(hexa)decimal ECID to look for\n");
    printf("\n");
}

#define OPTS    "sk:b:e:"

int main(int argc, char *const *argv) {
    const char *program_name = argv[0];
    uint8_t  kbag[KBAG_SIZE] = { 0 };
    uint8_t *kbags = NULL;
    uint64_t num = 0;
    uint64_t ecid = 0;
    bool decrypt = false;
    bool benchmark = false;
    bool sep = false;

    ANYA_INFO("%s", BUILD_TAG);
    ANYA_INFO("made by john (@nyan_satan)\n");

    int opt = 0;
    while ((opt = getopt(argc, argv, OPTS)) != -1) {
        switch (opt) {
            case 'k': {
                if (strlen(optarg) == KBAG_SIZE * 2) {
                    if (str2hex(KBAG_SIZE, kbag, optarg) != KBAG_SIZE) {
                        ANYA_ERROR("invalid KBAG");
                        return -1;
                    }
                } else {
                    ANYA_ERROR("KBAG must be 48 bytes in length");
                    return -1;
                }

                decrypt = true;
                break;
            }
    
            case 'b': {
                char *end_ptr;
                num = strtoull(optarg, &end_ptr, 0);
                if (*end_ptr != '\0') {
                    ANYA_ERROR("invalid benchmark NUM");
                    return -1;
                }

                if (num == 0) {
                    ANYA_ERROR("benchmark NUM cannot be zero");
                    return -1;
                }

                benchmark = true;
                break;
            }
    
            case 's': {
                sep = true;
                break;
            }
    
            case 'e': {
                char *end_ptr;
                ecid = strtoull(optarg, &end_ptr, 0);
                if (*end_ptr != '\0') {
                    ANYA_ERROR("invalid ECID");
                    return -1;
                }

                break;
            }
    
            case '?':
            default: {
                usage(program_name);
                return -1;
            }
        }
    }

    if (!decrypt && !benchmark) {
        ANYA_ERROR("neither benchmark nor KBAG set\n");
        usage(program_name);
        return -1;
    }

    if (decrypt && benchmark) {
        ANYA_ERROR("both benchmark and KBAG set\n");
        usage(program_name);
        return -1;
    }

    anya_error_t error;
    anya_device_t *dev;

    if (open_device(&dev, ecid) != 0) {
        return -1;
    }

    int ret = -1;

    if (sep) {
        bool result = false;

        if ((error = anya_ping_sep(dev, &result)) != ANYA_E_SUCCESS) {
            ANYA_ERROR("failed to check SEP availability - %s", anya_strerror(error));
            goto out;
        }

        if (!result) {
            ANYA_ERROR("SEP is unavailable");
            goto out;
        }

        ANYA_INFO("will use SEP GID");
    }

    if (decrypt) {
        uint8_t key[KBAG_SIZE] = { 0 };
        if ((error = anya_decrypt(dev, kbag, key, 1, sep)) != ANYA_E_SUCCESS) {
            ANYA_ERROR("failed to decrypt KBAG - %s", anya_strerror(error));
            goto out;
        }

        char str_key[KBAG_SIZE * 2 + 1] = { 0 };
        hex2str(str_key, KBAG_SIZE, key);
        ANYA_SUCCESS("%s", str_key);

    } else if (benchmark) {
        uint8_t *kbags = malloc(num * KBAG_SIZE);
        if (!kbags) {
            ANYA_ERROR("out of memory!");
            goto out;
        }

        arc4random_buf(kbags, num * KBAG_SIZE);

        ANYA_INFO("decrypting...");

        struct timeval st, et;
        gettimeofday(&st, NULL);

        if ((error = anya_decrypt(dev, kbags, kbags, num, sep)) != ANYA_E_SUCCESS) {
            ANYA_ERROR("failed to decrypt KBAGs - %s", anya_strerror(error));
            goto out;
        } 

        gettimeofday(&et, NULL);
        long elapsed = ((et.tv_sec - st.tv_sec) * (1000 * 1000)) + (et.tv_usec - st.tv_usec);

        float elapsed_sec = (float)elapsed / (1000 * 1000);
        float average_per_sec = (float)num / elapsed_sec;

        ANYA_SUCCESS("decrypted %llu KBAGs in %.3f seconds, average - %.3f KBAGs/sec", num, elapsed_sec, average_per_sec);
    }

    ret = 0;

out:
    if (dev) {
        anya_close(&dev);
    }

    if (kbags) {
        free(kbags);
    }

    return ret;
}
