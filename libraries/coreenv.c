#include "cyonstd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* Get environment variable value.
   Caller may pass NULL for out_buf to query length.
   Returns 0 on success, errno-like on failure or ENOENT if not found. */
int cyon_env_get(const char *name, char *out_buf, size_t buf_len) {
    if (!name) return EINVAL;
    char *val = getenv(name);
    if (!val) return ENOENT;
    size_t need = strlen(val) + 1;
    if (!out_buf) return (int)need;
    if (buf_len < need) return ENAMETOOLONG;
    memcpy(out_buf, val, need);
    return 0;
}

/* Set environment variable. Overwrite if overwrite != 0.
   Returns 0 on success, errno-like on failure. */
int cyon_env_set(const char *name, const char *value, int overwrite) {
    if (!name || !value) return EINVAL;
    if (setenv(name, value, overwrite) != 0) return errno ? errno : -1;
    return 0;
}

/* Unset environment variable.
   Returns 0 on success, errno-like on failure. */
int cyon_env_unset(const char *name) {
    if (!name) return EINVAL;
    if (unsetenv(name) != 0) return errno ? errno : -1;
    return 0;
}

/* Expand environment variables in-place into an allocated string.
   Supports ${VAR} and $VAR forms. Caller must free returned pointer.
   Returns NULL on allocation error. */
char *cyon_env_expand(const char *input) {
    if (!input) return NULL;
    size_t in_len = strlen(input);
    size_t cap = in_len + 1;
    char *out = (char*)malloc(cap);
    if (!out) return NULL;

    size_t oi = 0;
    for (size_t i = 0; i < in_len; ++i) {
        char c = input[i];
        if (c == '$') {
            const char *start = input + i + 1;
            char namebuf[256];
            size_t ni = 0;
            if (*start == '{') {
                ++i;
                ++start;
                while (i + 1 < in_len && input[i+1] != '}' && ni + 1 < sizeof(namebuf)) {
                    ++i;
                    namebuf[ni++] = input[i];
                }
                /* skip closing brace if present */
                if (i + 1 < in_len && input[i+1] == '}') ++i;
            } else {
                size_t j = i + 1;
                while (j < in_len && ((input[j] >= 'A' && input[j] <= 'Z') || (input[j] >= 'a' && input[j] <= 'z') || (input[j] == '_') || (input[j] >= '0' && input[j] <= '9'))) {
                    if (ni + 1 < sizeof(namebuf)) namebuf[ni++] = input[j];
                    ++j;
                }
                i = j - 1;
            }
            namebuf[ni] = '\0';
            if (ni == 0) {
                /* treat lone $ as literal */
                if (oi + 1 >= cap) { cap *= 2; out = (char*)realloc(out, cap); if (!out) return NULL; }
                out[oi++] = '$';
            } else {
                char *val = getenv(namebuf);
                if (val) {
                    size_t vlen = strlen(val);
                    while (oi + vlen + 1 >= cap) { cap *= 2; out = (char*)realloc(out, cap); if (!out) return NULL; }
                    memcpy(out + oi, val, vlen);
                    oi += vlen;
                }
            }
        } else {
            if (oi + 2 >= cap) { cap *= 2; out = (char*)realloc(out, cap); if (!out) return NULL; }
            out[oi++] = c;
        }
    }

    out[oi] = '\0';
    return out;
}

/* Load simple .env file with lines KEY=VALUE, ignores comments starting with #.
   Returns 0 on success; stops and returns errno-like on I/O error.
   Lines longer than 1024 are truncated. */
int cyon_env_load_file(const char *path, int overwrite) {
    if (!path) return EINVAL;
    FILE *f = fopen(path, "r");
    if (!f) return errno ? errno : -1;
    char line[1024];
    int rc = 0;
    while (fgets(line, sizeof(line), f)) {
        /* trim leading whitespace */
        char *p = line;
        while (*p && (*p == ' ' || *p == '\t')) ++p;
        if (*p == '#' || *p == '\n' || *p == '\0') continue;
        /* find '=' */
        char *eq = strchr(p, '=');
        if (!eq) continue;
        *eq = '\0';
        char *key = p;
        char *val = eq + 1;
        /* trim key trailing whitespace */
        char *kend = key + strlen(key) - 1;
        while (kend > key && (*kend == ' ' || *kend == '\t')) { *kend = '\0'; --kend; }
        /* trim value newline and surrounding quotes */
        char *vend = val + strlen(val);
        while (vend > val && (vend[-1] == '\n' || vend[-1] == '\r')) --vend;
        *vend = '\0';
        if (*val == '"' && vend - val > 1 && vend[-1] == '"') {
            val++; vend[-1] = '\0';
        } else if (*val == '\'' && vend - val > 1 && vend[-1] == '\'') {
            val++; vend[-1] = '\0';
        }
        if (key[0] == '\0') continue;
        if (setenv(key, val, overwrite) != 0) { rc = errno ? errno : -1; break; }
    }
    fclose(f);
    return rc;
}