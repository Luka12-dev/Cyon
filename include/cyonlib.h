#ifndef CYONLIB_H
#define CYONLIB_H

/* Use C linkage when included from C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* Basic includes */
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <time.h>

/* Export macro for shared builds */
#ifndef CYON_API
# ifdef _WIN32
#  ifdef CYON_BUILD_DLL
#   define CYON_API __declspec(dllexport)
#  else
#   define CYON_API __declspec(dllimport)
#  endif
# else
#  define CYON_API __attribute__((visibility("default")))
# endif
#endif

/* Generic error codes follow errno.h conventions; functions return 0 on success. */

/* Opaque handle forward declarations */
typedef struct cyon_thread_s cyon_thread_t;
typedef struct cyon_mutex_s cyon_mutex_t;
typedef struct cyon_cond_s cyon_cond_t;

/* Small geometry types reused across GUI helpers */
typedef struct { int x; int y; int w; int h; } cyon_rect_t;
typedef struct { unsigned char r, g, b; } cyon_color_t;

/* ssize_t portability */
#ifndef _SSIZE_T_DEFINED
typedef long ssize_t;
#endif

#ifdef __cplusplus
}
#endif

#endif /* CYONLIB_H */