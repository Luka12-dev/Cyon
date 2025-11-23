#include "cyonstd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

/* Read an entire file into a malloc'd buffer.
   On success *out_buf will point to heap memory (caller must free),
   *out_size will hold size, and return 0. On failure return errno. */
int cyon_readfile(const char *path, unsigned char **out_buf, size_t *out_size) {
    if (!path || !out_buf || !out_size) return EINVAL;
    FILE *f = fopen(path, "rb");
    if (!f) return errno ? errno : -1;

    if (fseek(f, 0, SEEK_END) != 0) {
        int err = errno ? errno : -1;
        fclose(f);
        return err;
    }
    long size = ftell(f);
    if (size < 0) { fclose(f); return EIO; }
    rewind(f);

    unsigned char *buf = (unsigned char*)malloc((size_t)size + 1);
    if (!buf) { fclose(f); return ENOMEM; }

    size_t read = fread(buf, 1, (size_t)size, f);
    if (read != (size_t)size) {
        free(buf);
        fclose(f);
        return EIO;
    }
    buf[read] = '\\0';
    fclose(f);

    *out_buf = buf;
    *out_size = read;
    return 0;
}

/* Write buffer to file, overwrite by default.
   Returns 0 on success. */
int cyon_writefile(const char *path, const unsigned char *buf, size_t size) {
    if (!path || !buf) return EINVAL;
    FILE *f = fopen(path, "wb");
    if (!f) return errno ? errno : -1;

    size_t written = fwrite(buf, 1, size, f);
    fclose(f);
    if (written != size) return EIO;
    return 0;
}

/* Check if path exists and whether it's a file or directory.
   type_out: 0 = not exist, 1 = file, 2 = directory */
int cyon_path_type(const char *path, int *type_out) {
    if (!path || !type_out) return EINVAL;
    struct stat st;
    if (stat(path, &st) != 0) {
        *type_out = 0;
        return errno ? errno : -1;
    }
    if (S_ISREG(st.st_mode)) *type_out = 1;
    else if (S_ISDIR(st.st_mode)) *type_out = 2;
    else *type_out = 0;
    return 0;
}

/* Copy small files in a portable way. Returns 0 on success. */
int cyon_copyfile(const char *src, const char *dst) {
    unsigned char *buf = NULL;
    size_t size = 0;
    int rc = cyon_readfile(src, &buf, &size);
    if (rc != 0) return rc;
    rc = cyon_writefile(dst, buf, size);
    free(buf);
    return rc;
}