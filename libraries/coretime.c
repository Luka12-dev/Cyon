#include "cyonstd.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

/* Return current unix epoch seconds. */
long cyon_time_now_seconds(void) {
    return (long)time(NULL);
}

/* Format current time into ISO-8601 like string in buffer.
   Buffer should be at least 32 bytes. Returns 0 on success. */
int cyon_time_iso8601(char *buf, size_t buflen) {
    if (!buf || buflen < 32) return EINVAL;
    time_t t = time(NULL);
    struct tm tm;
    if (!localtime_r(&t, &tm)) return EINVAL;
    size_t n = strftime(buf, buflen, "%Y-%m-%dT%H:%M:%S%z", &tm);
    if (n == 0) return EINVAL;
    return 0;
}

/* Sleep for given milliseconds. Returns 0 on success. */
int cyon_sleep_ms(unsigned int ms) {
    if (ms == 0) return 0;
    /* Use nanosleep for better granularity. */
    struct timespec req;
    req.tv_sec = ms / 1000;
    req.tv_nsec = (ms % 1000) * 1000000;
    while (nanosleep(&req, &req) == -1) {
        if (errno == EINTR) continue;
        return errno ? errno : -1;
    }
    return 0;
}

/* Convert epoch seconds to year for quick queries. Returns year or -1 on error. */
int cyon_epoch_to_year(long epoch) {
    struct tm tm;
    if (!localtime_r((time_t*)&epoch, &tm)) return -1;
    return tm.tm_year + 1900;
}