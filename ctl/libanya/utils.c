// ========================= Utilities =========================
// =============== These 2 are stolen from @xerub ==============

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

