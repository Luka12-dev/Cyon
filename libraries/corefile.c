#include "cyonstd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

/* Append data to a file, creating it if necessary. Returns 0 on success. */
int cyon_appendfile(const char *path, const unsigned char *buf, size_t size) {
    if (!path || (!buf && size > 0)) return EINVAL;
    int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) return errno ? errno : -1;
    size_t written = 0;
    while (written < size) {
        ssize_t r = write(fd, buf + written, size - written);
        if (r < 0) {
            if (errno == EINTR) continue;
            close(fd);
            return errno ? errno : -1;
        }
        written += (size_t)r;
    }
    close(fd);
    return 0;
}

/* Atomic write: write to a temporary file then rename into place. */
int cyon_atomic_write(const char *path, const unsigned char *buf, size_t size) {
    if (!path || (!buf && size > 0)) return EINVAL;
    char tmp[1024];
    snprintf(tmp, sizeof(tmp), "%s.tmp.%d", path, (int)getpid());
    int fd = open(tmp, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return errno ? errno : -1;
    size_t written = 0;
    while (written < size) {
        ssize_t r = write(fd, buf + written, size - written);
        if (r < 0) {
            if (errno == EINTR) continue;
            close(fd);
            unlink(tmp);
            return errno ? errno : -1;
        }
        written += (size_t)r;
    }
    fsync(fd);
    close(fd);
    if (rename(tmp, path) != 0) {
        unlink(tmp);
        return errno ? errno : -1;
    }
    return 0;
}

/* Read lines from file and call cb for each line. cb returns 0 to continue, non-zero to stop. */
int cyon_readlines(const char *path, int (*cb)(const char *line, void *userdata), void *userdata) {
    if (!path || !cb) return EINVAL;
    FILE *f = fopen(path, "r");
    if (!f) return errno ? errno : -1;
    char *line = NULL;
    size_t len = 0;
    ssize_t r;
    int rc = 0;
    while ((r = getline(&line, &len, f)) != -1) {
        /* strip newline */
        if (r > 0 && (line[r-1] == '\n' || line[r-1] == '\r')) line[r-1] = '\0';
        int cres = cb(line, userdata);
        if (cres != 0) { rc = cres; break; }
    }
    free(line);
    fclose(f);
    return rc;
}

/* Tail last n lines by seeking from end. Returns 0 on success and calls cb for each line from oldest to newest. */
int cyon_tail(const char *path, size_t n, int (*cb)(const char *line, void *userdata), void *userdata) {
    if (!path || !cb || n == 0) return EINVAL;
    FILE *f = fopen(path, "r");
    if (!f) return errno ? errno : -1;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return errno ? errno : -1; }
    long pos = ftell(f);
    if (pos < 0) { fclose(f); return errno ? errno : -1; }
    size_t lines_found = 0;
    long cur = pos - 1;
    char c;
    /* walk backwards to find start of last n lines */
    while (cur >= 0 && lines_found <= n) {
        if (fseek(f, cur, SEEK_SET) != 0) break;
        if (fread(&c, 1, 1, f) != 1) break;
        if (c == '\n') lines_found++;
        cur--;
    }
    long start = cur + 2;
    if (start < 0) start = 0;
    if (fseek(f, start, SEEK_SET) != 0) { fclose(f); return errno ? errno : -1; }
    char *line = NULL; size_t len = 0; ssize_t r;
    while ((r = getline(&line, &len, f)) != -1) {
        if (r > 0 && (line[r-1] == '\n' || line[r-1] == '\r')) line[r-1] = '\0';
        int cres = cb(line, userdata);
        if (cres != 0) { free(line); fclose(f); return cres; }
    }
    free(line);
    fclose(f);
    return 0;
}

/* Get file size in bytes. Returns 0 on success. */
int cyon_file_size(const char *path, size_t *out_size) {
    if (!path || !out_size) return EINVAL;
    struct stat st;
    if (stat(path, &st) != 0) return errno ? errno : -1;
    *out_size = (size_t)st.st_size;
    return 0;
}

/* Check existence. Returns 1 if exists, 0 if not, negative on error. */
int cyon_file_exists(const char *path) {
    if (!path) return -1;
    struct stat st;
    if (stat(path, &st) != 0) {
        if (errno == ENOENT) return 0;
        return -1;
    }
    return 1;
}