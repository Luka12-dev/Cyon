#ifndef CYONCRYPTO_H
#define CYONCRYPTO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cyonlib.h"
#include <stdint.h>

/* Non-cryptographic hash (FNV-1a) */
CYON_API uint32_t cyon_fnv1a_32(const unsigned char *data, size_t len);

/* OS random */
CYON_API int cyon_getrandom(void *buf, size_t buflen);

/* Hex encoding */
CYON_API char *cyon_hex_encode(const unsigned char *data, size_t len);

/* Placeholder for stronger hashes; implementations optional */
CYON_API char *cyon_hash_md5(const unsigned char *data, size_t len);
CYON_API char *cyon_hash_sha256(const unsigned char *data, size_t len);

/* Random bytes helper for wrappers */
CYON_API void cyon_random_seed(uint64_t seed);

#ifdef __cplusplus
}
#endif

#endif /* CYONCRYPTO_H */