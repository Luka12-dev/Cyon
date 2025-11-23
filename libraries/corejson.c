#include "cyonstd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Minimal JSON encode for strings. Caller must free returned pointer. */
char *cyon_json_escape_string(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    /* Worst-case every char becomes escaped (2x) plus quotes and null. */
    size_t max = len * 2 + 3;
    char *out = (char*)malloc(max);
    if (!out) return NULL;

    char *p = out;
    *p++ = '\"';
    for (size_t i = 0; i < len; ++i) {
        unsigned char c = (unsigned char)s[i];
        /* Escape minimal set. */
        if (c == '\"') { *p++ = '\\'; *p++ = '\"'; }
        else if (c == '\\') { *p++ = '\\'; *p++ = '\\'; }
        else if (c == '\n') { *p++ = '\\'; *p++ = 'n'; }
        else if (c == '\r') { *p++ = '\\'; *p++ = 'r'; }
        else if (c == '\t') { *p++ = '\\'; *p++ = 't'; }
        else if (c < 0x20) {
            /* Control chars as \\u00XX */
            sprintf(p, "\\u%04x", c);
            p += 6;
        } else {
            *p++ = c;
        }
    }
    *p++ = '\"';
    *p = '\\0';
    /* Optionally shrink allocation. */
    size_t used = (size_t)(p - out) + 1;
    char *shrunk = (char*)realloc(out, used);
    return shrunk ? shrunk : out;
}

/* Very small JSON "is-object-like" detector:
   returns 1 if string starts with '{' after whitespace, otherwise 0 */
int cyon_json_looks_like_object(const char *s) {
    if (!s) return 0;
    while (isspace((unsigned char)*s)) ++s;
    return *s == '{' ? 1 : 0;
}