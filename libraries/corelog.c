#include "cyonstd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

/* Log levels. */
enum { CYON_LOG_DEBUG = 10, CYON_LOG_INFO = 20, CYON_LOG_WARN = 30, CYON_LOG_ERROR = 40 };

/* Logger state. */
typedef struct {
    FILE *fp;
    char *path;
    int level;
    size_t max_size;
    pthread_mutex_t lock;
} cyon_logger_t;

static cyon_logger_t *global_logger = NULL;

/* Format timestamp ISO-8601 into buf of size >=32. */
static void cyon_log_timestamp(char *buf, size_t buflen) {
    time_t t = time(NULL);
    struct tm tm;
    localtime_r(&t, &tm);
    strftime(buf, buflen, "%Y-%m-%dT%H:%M:%S%z", &tm);
}

/* Internal rotate helper: rename file to .1 and truncate initial. */
static int cyon_log_rotate_if_needed(cyon_logger_t *lg) {
    if (!lg || !lg->path) return EINVAL;
    struct stat st;
    if (stat(lg->path, &st) != 0) {
        if (errno == ENOENT) return 0;
        return errno ? errno : -1;
    }
    if ((size_t)st.st_size <= lg->max_size) return 0;
    /* close current file pointer */
    if (lg->fp) fclose(lg->fp);
    /* rotate existing .1 */
    char oldpath[1024];
    char newpath[1024];
    snprintf(oldpath, sizeof(oldpath), "%s.1", lg->path);
    /* remove previous rotated if exists */
    unlink(oldpath);
    /* rename current to .1 */
    if (rename(lg->path, oldpath) != 0) {
        /* attempt best-effort reopen original */
    }
    /* reopen log file */
    lg->fp = fopen(lg->path, "a");
    if (!lg->fp) return errno ? errno : -1;
    return 0;
}

/* Initialize global logger. If path is NULL uses stderr.
   max_size in bytes triggers rotation when exceeded (0 = no rotation). */
int cyon_log_init(const char *path, int level, size_t max_size) {
    cyon_logger_t *lg = malloc(sizeof(cyon_logger_t));
    if (!lg) return ENOMEM;
    lg->path = NULL;
    lg->level = level;
    lg->max_size = max_size;
    pthread_mutex_init(&lg->lock, NULL);
    if (path) {
        lg->path = strdup(path);
        lg->fp = fopen(path, "a");
        if (!lg->fp) { free(lg->path); free(lg); return errno ? errno : -1; }
    } else {
        lg->fp = stderr;
    }
    global_logger = lg;
    return 0;
}

/* Close logger and free resources. */
int cyon_log_close(void) {
    if (!global_logger) return 0;
    pthread_mutex_lock(&global_logger->lock);
    if (global_logger->fp && global_logger->fp != stderr) fclose(global_logger->fp);
    if (global_logger->path) free(global_logger->path);
    pthread_mutex_unlock(&global_logger->lock);
    pthread_mutex_destroy(&global_logger->lock);
    free(global_logger);
    global_logger = NULL;
    return 0;
}

/* Internal write helper. */
static int cyon_log_write(int level, const char *tag, const char *fmt, va_list ap) {
    if (!global_logger) return EINVAL;
    if (level < global_logger->level) return 0;
    pthread_mutex_lock(&global_logger->lock);
    if (global_logger->max_size > 0) cyon_log_rotate_if_needed(global_logger);
    char ts[64];
    cyon_log_timestamp(ts, sizeof(ts));
    fprintf(global_logger->fp, "%s [%s] %s: ", ts, tag ? tag : "app", (level == CYON_LOG_DEBUG) ? "DEBUG" : (level == CYON_LOG_INFO) ? "INFO" : (level == CYON_LOG_WARN) ? "WARN" : "ERROR");
    vfprintf(global_logger->fp, fmt, ap);
    fprintf(global_logger->fp, "\n");
    fflush(global_logger->fp);
    pthread_mutex_unlock(&global_logger->lock);
    return 0;
}

/* Public log functions. */
int cyon_log_debug(const char *tag, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    int rc = cyon_log_write(CYON_LOG_DEBUG, tag, fmt, ap);
    va_end(ap);
    return rc;
}
int cyon_log_info(const char *tag, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    int rc = cyon_log_write(CYON_LOG_INFO, tag, fmt, ap);
    va_end(ap);
    return rc;
}
int cyon_log_warn(const char *tag, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    int rc = cyon_log_write(CYON_LOG_WARN, tag, fmt, ap);
    va_end(ap);
    return rc;
}
int cyon_log_error(const char *tag, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    int rc = cyon_log_write(CYON_LOG_ERROR, tag, fmt, ap);
    va_end(ap);
    return rc;
}