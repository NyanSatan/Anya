#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <spawn.h>
#include <sys/time.h>
#include <mach-o/getsect.h>

#include "libanya/utils.h"
#include "libanya/anya.h"
#include "libanya/log.h"
#include "astris.h"
#include "batch.h"
#include "map.h"

// ======================== Misc ========================

static int open_anya_device(anya_device_t **dev, uint64_t ecid, bool *has_ecid) {
    anya_error_t error;

    if ((error = anya_open(dev, ecid)) != ANYA_E_SUCCESS) {
        ANYA_ERROR("failed to connect - %s", anya_strerror(error));
        return -1;
    }


    if ((error = anya_ping_sep(*dev, has_ecid)) != ANYA_E_SUCCESS) {
        ANYA_ERROR("failed to check SEP availability - %s", anya_strerror(error));
        anya_close(dev);
        return -1;
    }

    anya_print_device(*dev);

    return 0;
}

static int parse_uint64(const char *str, uint64_t *res) {
    char *end_ptr;
    *res = strtoull(str, &end_ptr, 0);
    if (*end_ptr != '\0') {
        return -1;
    }

    return 0;
}

static uint64_t now_usec() {
    struct timeval t = { 0 };
    gettimeofday(&t, NULL);
    return t.tv_sec * 1000 * 1000 + t.tv_usec;
}

static float elapsed_sec(uint64_t end, uint64_t start) {
    return (float)(end - start) / (1000 * 1000);
}

static int write_file(const char *path, const void *buf, size_t len) {
    int fd = open(path, O_WRONLY | O_CREAT, 0644);
    if (fd < 0) {
        ANYA_ERROR("failed to open %s for writing", path);
        return -1;
    }

    int w = write(fd, buf, len);
    close(fd);

    if (w != len) {
        ANYA_ERROR("failed to write %s", path);
        return -1;
    }

    return 0;
}

// ======================== Build banner ========================

struct cs_blob {
    uint32_t type;
    uint32_t len;
};

#define CS_OPS_IDENTITY 11
int csops(pid_t pid, unsigned int  ops, void *useraddr, size_t usersize);

static void print_banner() {
    struct {
        struct cs_blob ops;
        char tag[128];
    } ops = { 0 };

    if (csops(getpid(), CS_OPS_IDENTITY, &ops, sizeof(ops)) < 0) {
        ANYA_WARNING("csops() failed (?!)");
    } else {
        ANYA_INFO("%s", ops.tag);
    }

    ANYA_INFO("made by john (@nyan_satan)\n");
}

// ======================== Main logic ========================

static void usage(const char *program_name) {
    printf("usage: %s VERB [ARGs]\n", program_name);
    printf("\n");

    printf("load\n");
    printf("  brings device up to Anya mode\n");
    printf("    -p PROBE\tspecify Astris probe name\n");
    printf("    -s PATH\toverride Anya script\n");
    printf("    -c PATH\toverride Anya DFU shellcode\n");
    printf("\n");

    printf("kbag <KBAG>\n");
    printf("  decrypts a single KBAG\n");
    printf("    -s\t\tuse SEP GID\n");
    printf("    -e ECID\t(hexa)decimal ECID to look for\n");
    printf("\n");

    printf("batch <IN> <OUT>\n");
    printf("  decrypts a JSON batch of KBAGs\n");
    printf("    -s\t\tuse SEP GID\n");
    printf("    -e ECID\t(hexa)decimal ECID to look for\n");
    printf("\n");

    printf("bench <COUNT>\n");
    printf("  decrypts COUNT random KBAGs to test performance\n");
    printf("    -s\t\tuse SEP GID\n");
    printf("    -e ECID\t(hexa)decimal ECID to look for\n");
    printf("\n");
}

#define LOAD_ARGS   ":p:s:c:"
#define TEMP_FILE   "/tmp/anya.XXXXXX"

int do_load(int argc, char *const *argv) {
    int ret = -1;

    char *probe = NULL;
    char *script_path = NULL;
    char *shellcode_path = NULL;

    bool script_needs_delete = false;
    bool shellcode_needs_delete = false;

    char *soc = NULL;
    char *rev = NULL;

    bool astris_inited = astris_api_init() == 0;
    bool map_inited = platform_map_init() == 0;

    char c;
    while ((c = getopt(argc, argv, LOAD_ARGS)) != -1) {
        switch (c) {
            case 'p': {
                probe = strdup(optarg);
                break;
            }

            case 's': {
                script_path = strdup(optarg);
                break;
            }

            case 'c': {
                shellcode_path = strdup(optarg);
                break;
            }

            case ':': {
                ANYA_ERROR("-%c needs argument", optopt);
                return -1;
            }

            case '?': {
                ANYA_ERROR("unknown argument -%c", optopt);
                return -1;
            }

            default:
                abort();
        }
    }

    if (!astris_inited && !probe) {
        ANYA_ERROR("Astris library could not be initialized, select probe manually (-p arg)");
        return -1;
    }

    if (!map_inited && (!script_path || !shellcode_path)) {
        ANYA_ERROR("no platform map in this build, select Astris script & shellcode manually (-s & -c args)");
        return -1;
    }

    if (!probe) {
        if (astris_probe_pick(&probe) != 0) {
            goto out;
        }
    }

    if (!script_path || !shellcode_path) {
        if (astris_soc_identify(probe, &soc, &rev) != 0) {
            goto out;
        }

        if (!platform_map_soc_supported(soc, rev)) {
            ANYA_ERROR("%s %s is not supported by this version of Anya", soc, rev);
            goto out;
        }

        ANYA_MISC("Detected %s %s", soc, rev);
    }

    if (!script_path) {
        script_path = strdup(TEMP_FILE);
        mktemp(script_path);

        const void *script = NULL;
        size_t script_len = 0;

        if (platform_map_select_script(soc, &script, &script_len) != 0) {
            ANYA_ERROR("BUG: platform is supported, but there is no corresponding script?!");
            goto out;
        }

        if (write_file(script_path, script, script_len) != 0) {
            goto out;
        }

        script_needs_delete = true;
    }

    if (!shellcode_path) {
        shellcode_path = strdup(TEMP_FILE);
        mktemp(shellcode_path);

        const void *shellcode = NULL;
        size_t shellcode_len = 0;

        if (platform_map_select_payload(soc, rev, &shellcode, &shellcode_len) != 0) {
            ANYA_ERROR("BUG: platform is supported, but there is no corresponding payload?!");
            goto out;
        }

        if (write_file(shellcode_path, shellcode, shellcode_len) != 0) {
            goto out;
        }

        shellcode_needs_delete = true;
    }

    ANYA_INFO("\n============ Launching Astris ============");

    uint64_t start = now_usec();

    char *astris_argv[] = {
        "astris",
        "--script",
        script_path,
        NULL
    };

    setenv("ANYA_PAYLOAD", shellcode_path, 1);

    extern char **environ;

    pid_t pid;
    int spawn = posix_spawnp(&pid, astris_argv[0], NULL, NULL, astris_argv, environ);
    if (spawn != 0) {
        ANYA_ERROR("failed to spawn Astris (%d)", spawn);
        goto out;
    }

    int status = 0;
    waitpid(pid, &status, 0);

    ANYA_INFO("============ Astris finished ============");

    if (WIFSIGNALED(status)) {
        ANYA_ERROR("Astris crashed with signal %d lol", WTERMSIG(status));
        goto out;
    }

    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
        ANYA_ERROR("Astris exited with code %d", WEXITSTATUS(status));
        goto out;
    }

    uint64_t end = now_usec();
    float elapsed = elapsed_sec(end, start);

    ANYA_SUCCESS("\nAnya initialization finished!");
    ANYA_SUCCESS("took - %.2fs", elapsed);

out:
    if (script_needs_delete) {
        unlink(script_path);
    }

    if (shellcode_needs_delete) {
        unlink(shellcode_path);
    }

    free(script_path);
    free(shellcode_path);

    free(probe);
    free(soc);
    free(rev);

    return ret;
}

#define COMMON_DECRYPT_ARGS   ":se:"

static int _parse_decrypt_args(int argc, char *const *argv, bool *sep, uint64_t *ecid) {
    char c;
    while ((c = getopt(argc, argv, COMMON_DECRYPT_ARGS)) != -1) {
        switch (c) {
            case 's': {
                *sep = true;
                break;
            }

            case 'e': {
                if (parse_uint64(optarg, ecid) != 0) {
                    ANYA_ERROR("invalid ECID");
                    return -1;
                }

                break;
            }

            case ':': {
                ANYA_ERROR("-%c needs argument\n", optopt);
                return -1;
            }

            case '?': {
                ANYA_ERROR("unknown argument -%c\n", optopt);
                return -1;
            }

            default:
                abort();
        }
    }

    return 0;
}

int do_kbag(const char *kbag, int argc, char *const *argv) {
    uint8_t kbag_decoded[KBAG_SIZE] = { 0 };
    uint8_t key[KBAG_SIZE] = { 0 };
    char str_key[KBAG_SIZE * 2 + 1] = { 0 };
    bool sep = false;
    uint64_t ecid = 0;
    anya_error_t error = ANYA_E_SUCCESS;
    anya_device_t *dev = NULL;
    bool has_sep = false;
    int ret = -1;

    if (strlen(kbag) != KBAG_SIZE * 2) {
        ANYA_ERROR("KBAG must be 48 bytes in length");
        goto out;
    }

    if (str2hex(KBAG_SIZE, kbag_decoded, kbag) != KBAG_SIZE) {
        ANYA_ERROR("invalid KBAG");
        goto out;
    }

    if (_parse_decrypt_args(argc, argv, &sep, &ecid) != 0) {
        goto out;
    }

    if (open_anya_device(&dev, ecid, &has_sep) != 0) {
        goto out;
    }

    if (sep) {
        if (!has_sep) {
            ANYA_ERROR("SEP is unavailable");
            goto out;
        }

        ANYA_MISC("will use SEP GID");
    }

    if ((error = anya_decrypt(dev, kbag_decoded, key, 1, sep)) != ANYA_E_SUCCESS) {
        ANYA_ERROR("failed to decrypt KBAG - %s", anya_strerror(error));
        goto out;
    }

    hex2str(str_key, KBAG_SIZE, key);
    fprintf(stdout, "%s\n", str_key);

    ret = 0;

out:
    if (dev) {
        anya_close(&dev);
    }

    return ret;
}

int do_batch(const char *in, const char *out, int argc, char *const *argv) {
    bool sep = false;
    uint64_t ecid = 0;
    void *batch_ctx = NULL;
    void *decoded_kbags = NULL;
    anya_error_t error = ANYA_E_SUCCESS;
    anya_device_t *dev = NULL;
    bool has_sep = false;
    int ret = -1;

    if (_parse_decrypt_args(argc, argv, &sep, &ecid) != 0) {
        return -1;
    }

    if (batch_init(in, &batch_ctx) != 0) {
        goto out;
    }

    int kbag_count = batch_get_count(batch_ctx);

    decoded_kbags = malloc(kbag_count * KBAG_SIZE);
    if (!decoded_kbags) {
        ANYA_ERROR("out of memory");
        goto out;
    }

    for (int i = 0; i < kbag_count; i++) {
        batch_get_kbag(batch_ctx, i, decoded_kbags + i * KBAG_SIZE);
    }

    if (open_anya_device(&dev, ecid, &has_sep) != 0) {
        goto out;
    }

    if (sep) {
        if (!has_sep) {
            ANYA_ERROR("SEP is unavailable");
            goto out;
        }

        ANYA_MISC("will use SEP GID");
    }

    uint64_t start = now_usec();

    if ((error = anya_decrypt(dev, decoded_kbags, decoded_kbags, kbag_count, sep)) != ANYA_E_SUCCESS) {
        ANYA_ERROR("failed to decrypt KBAGs - %s", anya_strerror(error));
        goto out;
    }

    for (int i = 0; i < kbag_count; i++) {
        batch_set_key(batch_ctx, i, decoded_kbags + i * KBAG_SIZE);
    }

    if (batch_write(batch_ctx, out)) {
        goto out;
    }

    uint64_t end = now_usec();

    float elapsed = elapsed_sec(end, start);
    float average_per_sec = (float)kbag_count / elapsed;

    ANYA_SUCCESS("successfully decrypted %d KBAGs in %.3f seconds (%.3f KBAGs/sec)", kbag_count, elapsed, average_per_sec);

    ret = 0;

out:
    free(decoded_kbags);

    if (batch_ctx) {
        batch_quiesce(&batch_ctx);
    }

    if (dev) {
        anya_close(&dev);
    }

    return ret;
}

int do_bench(const char *count, int argc, char *const *argv) {
    bool sep = false;
    uint64_t ecid = 0;
    uint64_t _count = 0;
    anya_error_t error = ANYA_E_SUCCESS;
    anya_device_t *dev = NULL;
    bool has_sep = false;
    int ret = -1;

    if (parse_uint64(count, &_count) != 0) {
        ANYA_ERROR("invalid count");
        return -1;
    }

    if (_parse_decrypt_args(argc, argv, &sep, &ecid) != 0) {
        return -1;
    }

    if (open_anya_device(&dev, ecid, &has_sep) != 0) {
        goto out;
    }

    if (sep) {
        if (!has_sep) {
            ANYA_ERROR("SEP is unavailable");
            goto out;
        }

        ANYA_MISC("will use SEP GID");
    }

    uint8_t *kbags = malloc(_count * KBAG_SIZE);
    if (!kbags) {
        ANYA_ERROR("out of memory!");
        goto out;
    }

    arc4random_buf(kbags, _count * KBAG_SIZE);

    ANYA_MISC("decrypting...");

    uint64_t start = now_usec();

    if ((error = anya_decrypt(dev, kbags, kbags, _count, sep)) != ANYA_E_SUCCESS) {
        ANYA_ERROR("failed to decrypt KBAGs - %s", anya_strerror(error));
        goto out;
    }

    uint64_t end = now_usec();

    float elapsed = elapsed_sec(end, start);
    float average_per_sec = (float)_count / elapsed;

    ANYA_SUCCESS("decrypted %llu KBAGs in %.3f seconds, average - %.3f KBAGs/sec", _count, elapsed, average_per_sec);

    ret = 0;

out:
    if (dev) {
        anya_close(&dev);
    }

    return ret;
}

int main(int argc, char *const *argv) {
    print_banner();

    if (argc < 2) {
        goto usage;
    }

    opterr = 0;

    const char *verb = argv[1];

    if (strcmp(verb, "load") == 0) {
        return do_load(argc-1, argv+1);
    }

    if (strcmp(verb, "kbag") == 0) {
        if (argc < 3) {
            goto usage;
        }
        return do_kbag(argv[2], argc-2, argv+2);
    }

    if (strcmp(verb, "batch") == 0) {
        if (argc < 4) {
            goto usage;
        }
        return do_batch(argv[2], argv[3], argc-3, argv+3);
    }

    if (strcmp(verb, "bench") == 0) {
        if (argc < 3) {
            goto usage;
        }
        return do_bench(argv[2], argc-2, argv+2);
    }

usage:
    usage(argv[0]);
    return -1;
}
