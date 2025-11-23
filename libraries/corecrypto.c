#include "cyonstd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

/* Simple non-cryptographic FNV-1a 32-bit hash for small inputs.
   Useful as a checksum or table key, not for security. */
uint32_t cyon_fnv1a_32(const unsigned char *data, size_t len) {
    const uint32_t prime = 16777619u;
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < len; ++i) {
        hash ^= (uint32_t)data[i];
        hash *= prime;
    }
    return hash;
}

/* Fill buffer with OS-provided randomness (/dev/urandom on POSIX).
   Returns 0 on success. */
int cyon_getrandom(void *buf, size_t buflen) {
    if (!buf && buflen > 0) return EINVAL;
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) return errno ? errno : -1;
    size_t offset = 0;
    while (offset < buflen) {
        ssize_t r = read(fd, (unsigned char*)buf + offset, buflen - offset);
        if (r <= 0) {
            if (errno == EINTR) continue;
            close(fd);
            return errno ? errno : -1;
        }
        offset += (size_t)r;
    }
    close(fd);
    return 0;
}

/* Simple hex encoder. Caller must free returned string. */
char *cyon_hex_encode(const unsigned char *data, size_t len) {
    if (!data && len > 0) return NULL;
    const char *hex = "0123456789abcdef";
    char *out = (char*)malloc(len * 2 + 1);
    if (!out) return NULL;
    for (size_t i = 0; i < len; ++i) {
        out[i*2]     = hex[(data[i] >> 4) & 0xF];
        out[i*2 + 1] = hex[data[i] & 0xF];
    }
    out[len*2] = '\\0';
    return out;
}