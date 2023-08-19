#include "time.h"

bool time_has_elapsed(uint64_t start, uint64_t timeout) {
    return (system_time() - start) >= timeout;
}
