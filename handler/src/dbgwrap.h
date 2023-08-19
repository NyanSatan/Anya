#ifndef DBGWRAP_H
#define DBGWRAP_H

#include <stdint.h>
#include <stdbool.h>

#define DBGWRAP_OFFSET  (0x30000)

int  dbgwrap_halt(uintptr_t dbgwrap_base, uint64_t timeout);
bool dbgwrap_is_halted(uintptr_t dbgwrap_base);

#endif
