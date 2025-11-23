#ifndef CYONFS_H
#define CYONFS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cyonlib.h"

/* Path and directory helpers */
CYON_API int cyon_getcwd(char *buf, size_t len);
CYON_API int cyon_mkdir_p(const char *path);
CYON_API int cyon_remove(const char *path);
CYON_API int cyon_listdir(const char *path, int (*cb)(const char *name, void *userdata), void *userdata);
CYON_API int cyon_resolve_path(const char *path, char *outbuf, size_t outlen);

/* File read/write helpers */
CYON_API int cyon_readfile(const char *path, unsigned char **out_buf, size_t *out_size);
CYON_API int cyon_writefile(const char *path, const unsigned char *buf, size_t size);
CYON_API int cyon_copyfile(const char *src, const char *dst);
CYON_API int cyon_path_type(const char *path, int *type_out);

/* Advanced file helpers */
CYON_API int cyon_appendfile(const char *path, const unsigned char *buf, size_t size);
CYON_API int cyon_atomic_write(const char *path, const unsigned char *buf, size_t size);
CYON_API int cyon_readlines(const char *path, int (*cb)(const char *line, void *userdata), void *userdata);
CYON_API int cyon_tail(const char *path, size_t n, int (*cb)(const char *line, void *userdata), void *userdata);
CYON_API int cyon_file_size(const char *path, size_t *out_size);
CYON_API int cyon_file_exists(const char *path);

#ifdef __cplusplus
}
#endif

#endif /* CYONFS_H */