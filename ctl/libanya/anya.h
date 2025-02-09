#ifndef ANYA_H
#define ANYA_H

#include <stdint.h>
#include <stdbool.h>
#include <lilirecovery.h>

#define CPFM_SECURE_STATUS      (1 << 0)
#define CPFM_PRODUCTION_STATUS  (1 << 1)

typedef struct anya_device anya_device_t;

typedef enum {
    ANYA_E_SUCCESS = 0,
    ANYA_E_OUT_OF_MEMORY,
    ANYA_E_NO_DEVICE,
    ANYA_E_NOT_ANYA_DEVICE,
    ANYA_E_USB_ERROR,
    ANYA_E_HANDLER_ERROR,
    ANYA_E_UNKNOWN_ERROR
} anya_error_t;

#define KBAG_SIZE   0x30

anya_error_t anya_open(anya_device_t **dev, uint64_t ecid);
anya_error_t anya_close(anya_device_t **dev);

/* these are for the Python frontend */
uint16_t anya_get_cpid(anya_device_t *dev);
uint8_t  anya_get_cpfm(anya_device_t *dev);
uint64_t anya_get_ecid(anya_device_t *dev);

anya_error_t anya_ping_sep(anya_device_t *dev, bool *res);

anya_error_t anya_decrypt(anya_device_t *dev, uint8_t kbags[], uint8_t keys[], size_t count, bool sep);

anya_error_t anya_reboot(anya_device_t *dev);

void anya_print_device(anya_device_t *dev);

const char *anya_strerror(anya_error_t error);

#endif
