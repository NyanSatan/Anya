#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/time.h>
#include "libanya/anya.h"

/*
 * These 2 are stolen from @xerub
 */

long str2hex(unsigned long long max, unsigned char *buf, const char *str) {
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

void hex2str(char *str, int buflen, const unsigned char *buf) {
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

int open_device(anya_device_t **dev, uint64_t ecid) {
    anya_error_t error;

    if ((error = anya_open(dev, ecid)) != ANYA_E_SUCCESS) {
        printf("failed to open device, reason: %s\n", anya_strerror(error));
        return -1;
    }

    printf("found: ");
    anya_print_device(*dev);

    return 0;
}

void usage(const char *program_name) {
    printf("usage: %s ARG[s]\n", program_name);
    printf("\n");
    printf("where ARG[s] must be one of the following:\n");
    printf("\t-k KBAG\tspecifies KBAG to be decrypted\n");
    printf("\t-b NUM\truns benchmark with NUM random KBAGs\n");
    printf("\t-s\tuses SEP GID (if possible)\n");
    printf("\n");
    printf("you can also use this one with both of the above:\n");
    printf("\t-e ECID\t(hexa)decimal ECID to look for\n");
    printf("\n");
}

typedef enum {
    ANYACTL_VERB_UNDEFINED = -1,
    ANYACTL_VERB_DECRYPT = 0,
    ANYACTL_VERB_BENCHMARK
} ctl_verb_t;

int main(int argc, char const *argv[]) {
    const char *program_name = argv[0];
    ctl_verb_t verb = ANYACTL_VERB_UNDEFINED;
    uint8_t  kbag[KBAG_SIZE] = {0};
    uint64_t num = 0;
    uint64_t ecid = 0;
    bool sep = false;

    for (int i = 1; i < argc; i++) {
        bool arg_decrypt = strcmp(argv[i], "-k") == 0;
        bool arg_benchmark = strcmp(argv[i], "-b") == 0;
        bool arg_ecid = strcmp(argv[i], "-e") == 0;
        bool arg_sep = strcmp(argv[i], "-s") == 0;

        if (!arg_decrypt && !arg_benchmark && !arg_ecid && !arg_sep) {
            printf("undefined argument: %s\n", argv[i]);
            usage(program_name);
            return -1;
        }

        if (verb != ANYACTL_VERB_UNDEFINED && (arg_benchmark || arg_decrypt)) {
            printf("effective argument already set, can't have 2\n");
            return -1;    
        }

        if (arg_sep) {
            sep = true;
            continue;
        }

        i++;

        if (i == argc) {
            usage(program_name);
            return -1;
        }

        if (arg_decrypt) {
            verb = ANYACTL_VERB_DECRYPT;

            if (strlen(argv[i]) == KBAG_SIZE * 2) {
                if (str2hex(KBAG_SIZE, kbag, argv[i]) != KBAG_SIZE) {
                    printf("invalid KBAG\n");
                    return -1;
                } else {
                    continue;
                }
            } else {
                printf("KBAG must be 48 bytes in length\n");
                return -1;
            }

        } else if (arg_benchmark) {
            verb = ANYACTL_VERB_BENCHMARK;

            char *end_ptr;
            num = strtoull(argv[i], &end_ptr, 0);
            if (*end_ptr != '\0') {
                printf("invalid NUM\n");
                return -1;
            }

            if (num == 0) {
                printf("NUM cannot be zero\n");
                return -1;
            }

        } else if (arg_ecid) {
            if (ecid) {
                printf("ECID already set\n");
                return -1;
            }

            char *end_ptr;
            ecid = strtoull(argv[i], &end_ptr, 0);
            if (*end_ptr != '\0') {
                printf("invalid ECID\n");
                return -1;
            }
        }

    }

    if (verb == ANYACTL_VERB_UNDEFINED) {
        usage(program_name);
        return -1;
    }

    anya_error_t error;
    anya_device_t *dev;

    if (open_device(&dev, ecid) != 0) {
        return -1;
    }

    if (sep) {
        bool result = false;

        if (anya_ping_sep(dev, &result) != ANYA_E_SUCCESS) {
            printf("failed to check SEP availability\n");
            anya_close(&dev);
            return -1;
        }

        if (!result) {
            printf("SEP is unavailable\n");
            anya_close(&dev);
            return -1;
        }
    }

    switch (verb) {
        case ANYACTL_VERB_DECRYPT: {
            uint8_t key[KBAG_SIZE];
            if ((error = anya_decrypt_internal(dev, kbag, key, sep)) != ANYA_E_SUCCESS) {
                printf("failed to decrypt KBAG, reason: %s\n", anya_strerror(error));
                anya_close(&dev);
                return -1;
            }

            char str_key[KBAG_SIZE * 2 + 1];
            hex2str(str_key, KBAG_SIZE, key);
            printf("%s\n", str_key);

            break;
        }

        case ANYACTL_VERB_BENCHMARK: {
            uint8_t *kbags = malloc(num * KBAG_SIZE);
            if (!kbags) {
                printf("out of memory!");
                anya_close(&dev);
                return -1;
            }

            arc4random_buf(kbags, num * KBAG_SIZE);

            printf("decrypting...\n");

            uint8_t key[KBAG_SIZE];

            struct timeval st, et;
            gettimeofday(&st, NULL);

            for (uint64_t i = 0; i < num; i++) {
                if ((error = anya_decrypt_internal(dev, kbags + (i * KBAG_SIZE), key, sep)) != ANYA_E_SUCCESS) {
                    printf("failed to decrypt KBAG, reason: %s\n", anya_strerror(error));
                    anya_close(&dev);
                    return -1;
                }   
            }

            gettimeofday(&et, NULL);
            long elapsed = ((et.tv_sec - st.tv_sec) * 1000000) + (et.tv_usec - st.tv_usec);

            float elapsed_sec = (float)elapsed / (1000 * 1000);
            float average_per_sec = (float)num / elapsed_sec;

            printf("decrypted %llu KBAGs in %f seconds, average - %f KBAGs/sec\n", num, elapsed_sec, average_per_sec);

            break;
        }

        default:
            //just to silence the warning - we handle ANYACTL_VERB_UNDEFINED before
            break;
    }

    anya_close(&dev);

    return 0;
}
