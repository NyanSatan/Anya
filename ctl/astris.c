#include <stdio.h>
#include <dlfcn.h>
#include <string.h>
#include <stdlib.h>
#include <sys/param.h>
#include "astris.h"
#include "libanya/log.h"

// ======================== Astris API ========================
// ============== nothing was stolen here, no no ==============

typedef struct AXconn_s AXconn_t;

typedef enum {
    PROBE_STAT_NONE = 0,
    PROBE_STAT_IDLE = 1,
    PROBE_STAT_BUSY = 2
} AXProbeStatus_t;

typedef struct {
    char           *probeName;
    AXProbeStatus_t probeStatus;
    /* ... */
} AXprobe_t;

AXconn_t*   (*AXconn_create)(void);
int         (*AXconn_destroy)(AXconn_t *info);
int         (*AXconn_connect)(AXconn_t *info, const char *host, int port, unsigned int flags);
int         (*AXjtag_explore)(AXconn_t *info, unsigned int flags);
const char* (*AXconn_identify)(AXconn_t *info);
const char* (*AXconn_identify_rev)(AXconn_t *info);
int         (*AXconn_probelist)(AXconn_t *info, const char *host, AXprobe_t **probelist, int *probecount);
void        (*AXconn_probelist_free)(AXprobe_t *probelist, int probecount);
const char* (*AXversion_src_short_get)(void);
const char* (*AXstrerror)(int astrisErr);
int         (*AXbooster_init_with_paths)(AXconn_t *conn, char *boosterpack_paths[], int len);

#define ASTRIS_ERR_OK           (0)
#define AXJTAG_EXPLORE_GENTLY   (0x00000002)
#define AXJTAG_EXPLORE_APONLY   (0x00000010)

// ==================== Astris API linking ====================

#define SOFT_LINK(handle, symbol, fail_label) \
    do { \
        symbol = dlsym(handle, #symbol); \
        if (!symbol) { \
            ANYA_WARNING("symbol %s was not found!", #symbol); \
            goto fail_label; \
        } \
    } while (0)

#define ASTRIS_LIB_PATH "usr/local/lib/libastris-python-proxy.dylib"

static char boosterpack_root[MAXPATHLEN] = {0};

long astris_version = 0;

int astris_api_init(const char *astris_root) {
    int ret = -1;
    char *version = NULL;
    char astris_lib[MAXPATHLEN] = {0};

    snprintf(astris_lib, sizeof(astris_lib), "%s/%s", astris_root, ASTRIS_LIB_PATH);

    void *handle = dlopen(astris_lib, RTLD_LAZY);
    if (!handle) {
        ANYA_WARNING("couldn't open Astris library");
        ANYA_WARNING("%s", dlerror());
        goto out;
    }

    SOFT_LINK(handle, AXconn_create, out);
    SOFT_LINK(handle, AXconn_destroy, out);
    SOFT_LINK(handle, AXconn_connect, out);
    SOFT_LINK(handle, AXjtag_explore, out);
    SOFT_LINK(handle, AXconn_identify, out);
    SOFT_LINK(handle, AXconn_identify_rev, out);
    SOFT_LINK(handle, AXconn_probelist, out);
    SOFT_LINK(handle, AXconn_probelist_free, out);
    SOFT_LINK(handle, AXversion_src_short_get, out);
    SOFT_LINK(handle, AXstrerror, out);
    SOFT_LINK(handle, AXbooster_init_with_paths, out);

    version = strdup(AXversion_src_short_get());

    char *break_point = strchr(version, '.');
    if (!break_point) {
        /* for tags with major part only */
        break_point = strchr(version, 'b');
    }

    *break_point = '\0';

    char *stop = NULL;
    long version_num = strtol(version, &stop, 10);

    if (*stop != '\0') {
        ANYA_WARNING("failed to parse Astris version");
        goto out;
    }

    astris_version = version_num;

    strlcpy(boosterpack_root, astris_root, sizeof(boosterpack_root));

    ret = 0;

out:
    free(version);

    return ret;
}

// ===================== Actual routines =====================

int astris_soc_identify(const char *probe, char **soc, char **rev) {
    int ret = -1;
    AXconn_t *conn = NULL;

    /* if this starts with ':' Astris will destroy the string */
    char *_probe = strdup(probe);

    conn = AXconn_create();
    if (!conn) {
        ANYA_ERROR("failed to create Astris connection object");
        goto out;
    }

    char *boosterpack_paths[] = {
        boosterpack_root
    };

    /* gotta do it manually for custom Astris roots */
    AXbooster_init_with_paths(conn, boosterpack_paths, 1);

    ret = AXconn_connect(conn, _probe, 0, 0);
    if (ret != ASTRIS_ERR_OK) {
        ANYA_ERROR("cannot connect to Astris probe (%s)", AXstrerror(ret));
        goto out;
    }

    ret = AXjtag_explore(conn, AXJTAG_EXPLORE_APONLY);
    if (ret < ASTRIS_ERR_OK) {
        ANYA_ERROR("cannot explore target (%s)", AXstrerror(ret));
        goto out;
    }

    const char *_soc = AXconn_identify(conn);

    if (!_soc) {
        ANYA_ERROR("cannot identify SoC");
        goto out;
    }

    const char *_rev = NULL;
    bool tried = false;

    while (true) {
       _rev = AXconn_identify_rev(conn);

        if (!_rev) {
            if (!tried) {
                /* if we failed to detect revision, try to explore target further */
                ret = AXjtag_explore(conn, AXJTAG_EXPLORE_GENTLY);
                if (ret < ASTRIS_ERR_OK) {
                    ANYA_ERROR("cannot re-explore target (%s)", AXstrerror(ret));
                    goto out;
                }

                tried = true;
                continue;

            } else {
                ANYA_ERROR("cannot identify SoC revision");
                goto out;
            }
        }

        break;
    }

    *soc = strdup(_soc);
    *rev = strdup(_rev);

    ret = 0;

out:
    if (conn) {
        AXconn_destroy(conn);
    }

    free(_probe);

    return ret;
}

/* Later Dawn Astris added another field to AXprobe struct */
#define ASTRIS_VERSION_USBC_PROBE_THRESHOLD (2400)

/* XXX will break on 32-bit CPUs, but do we care? */
#define PROBE_AT_IDX(arr, idx) \
    ((AXprobe_t *)((void *)arr + idx * (astris_version > ASTRIS_VERSION_USBC_PROBE_THRESHOLD ? 24 : 16)))

int astris_probe_pick(char **probe) {
    int ret = -1;
    AXconn_t  *conn = NULL;
    AXprobe_t *probes = NULL;

    conn = AXconn_create();
    if (!conn) {
        ANYA_ERROR("failed to create Astris connection object");
        goto fail;
    }

    int count = 0;
    int astris_ret = AXconn_probelist(conn, NULL, &probes, &count);
    if (astris_ret != ASTRIS_ERR_OK) {
        ANYA_ERROR("cannot get probe list (%s)", AXstrerror(ret));
        goto fail;
    }

    if (!count) {
        ANYA_ERROR("no Astris probes connected!");
        goto fail;
    }

    int idx = -1;

    if (count == 1) {
        idx = 0;
        goto out;
    }

    while (1) {
        ANYA_INFO("Select probe from the list below:");

        for (int i = 0; i < count; i++) {
            fprintf(stderr, "\t%c) %s%s\n", i + 'a', PROBE_AT_IDX(probes, i)->probeName, PROBE_AT_IDX(probes, i)->probeStatus == PROBE_STAT_BUSY ? " [Busy]" : "");
        }

        char c = getchar();
        int _idx = c - 'a';

        if (c < 'a' || _idx >= count) {
            ANYA_WARNING("option out of range, try again\n");
            continue;
        }

        idx = _idx;
        break;
    }

out:
    if (PROBE_AT_IDX(probes, idx)->probeStatus == PROBE_STAT_BUSY) {
        ANYA_ERROR("probe %s is busy\n", PROBE_AT_IDX(probes, idx)->probeName);
        goto fail;
    }

    *probe = strdup(PROBE_AT_IDX(probes, idx)->probeName);

    ret = 0;

fail:
    if (probes) {
        AXconn_probelist_free(probes, count);
    }

    if (conn) {
        AXconn_destroy(conn);
    }

    return ret;
}
