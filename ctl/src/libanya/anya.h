#ifndef ANYA_H
#define ANYA_H

#include <stdint.h>
#include <libirecovery.h>

#define CPFM_SECURE_STATUS      (1 << 0)
#define CPFM_PRODUCTION_STATUS  (1 << 1)

typedef struct {
    irecv_client_t  conn;
    uint16_t        cpid;
    uint8_t         cpfm;
    uint64_t        ecid;
} anya_device_t;

typedef enum {
    ANYA_E_SUCCESS = 0,
    ANYA_E_OUT_OF_MEMORY,
    ANYA_E_NO_DEVICE,
    ANYA_E_NOT_ANYA_DEVICE,
    ANYA_E_USB_ERROR,
    ANYA_E_UNKNOWN_ERROR
} anya_error_t;

#define KBAG_SIZE   0x30

anya_error_t anya_open(anya_device_t **dev, uint64_t ecid);
anya_error_t anya_decrypt(anya_device_t *dev, uint8_t kbag[KBAG_SIZE], uint8_t key[KBAG_SIZE]);
anya_error_t anya_reboot(anya_device_t *dev);
anya_error_t anya_close(anya_device_t **dev);

void anya_print_device(anya_device_t *dev);

const char* anya_strerror(anya_error_t error);

#endif
