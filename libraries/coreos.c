#include "cyonstd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>

/* Return the current working directory into `buf` of size `len`.
   Returns 0 on success, non-zero on error. */
int cyon_getcwd(char *buf, size_t len) {
    /* Guard against invalid arguments. */
    if (!buf || len == 0) {
        return EINVAL;
    }

    /* Use POSIX getcwd where available. */
    if (getcwd(buf, len) == NULL) {
        /* Copy errno into return value for caller handling. */
        int err = errno ? errno : -1;
        /* Ensure a tidy empty string on failure. */
        if (len > 0) buf[0] = '\\0';
        return err;
    }

    return 0;
}

/* Make a directory at `path` with default permissions.
   Returns 0 on success, non-zero errno on failure. */
int cyon_mkdir_p(const char *path) {
    /* Simple wrapper that attempts to call mkdir and reports error. */
    if (!path) return EINVAL;
    int rc = mkdir(path, 0755);
    if (rc == 0) return 0;
    return errno ? errno : -1;
}

/* Remove file or empty directory. Returns 0 on success. */
int cyon_remove(const char *path) {
    if (!path) return EINVAL;
    int rc = remove(path);
    if (rc == 0) return 0;
    return errno ? errno : -1;
}

/* List directory entries by calling a user-provided callback for each name.
   The callback must return 0 to continue or non-zero to stop early. */
int cyon_listdir(const char *path, int (*cb)(const char *name, void *userdata), void *userdata) {
    if (!path || !cb) return EINVAL;
    DIR *d = opendir(path);
    if (!d) return errno ? errno : -1;

    struct dirent *ent;
    int rc = 0;
    while ((ent = readdir(d)) != NULL) {
        /* Skip dot entries by default. */
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
        int cb_rc = cb(ent->d_name, userdata);
        if (cb_rc != 0) { rc = cb_rc; break; }
    }

    closedir(d);
    return rc;
}

/* Resolve a path to an absolute path via realpath.
   Caller provides outbuf with outlen. Returns 0 on success. */
int cyon_resolve_path(const char *path, char *outbuf, size_t outlen) {
    if (!path || !outbuf || outlen == 0) return EINVAL;
    char tmp[PATH_MAX];
    if (!realpath(path, tmp)) {
        return errno ? errno : -1;
    }
    if (strlen(tmp) + 1 > outlen) return ENAMETOOLONG;
    strcpy(outbuf, tmp);
    return 0;
}