#ifndef CYONMEM_H
#define CYONMEM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cyonlib.h"

/* Simple allocation helpers could be added here later.
   For now keep placeholder API for potential runtime wrappers. */

/* Example: secure free (zero memory then free) */
CYON_API void cyon_secure_free(void *ptr, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* CYONMEM_H */