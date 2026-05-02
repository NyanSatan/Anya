#ifndef LOADER_H
#define LOADER_H

#include <stddef.h>

int  platform_map_init();
bool platform_map_soc_supported(const char *soc, const char *rev);
int  platform_map_select_script(const char *soc, const void **script, size_t *script_len);
int  platform_map_select_payload(const char *soc, const char *rev, const void **payload, size_t *payload_len);

#endif
