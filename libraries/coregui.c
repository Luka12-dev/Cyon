#include "cyonstd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <errno.h>

/* Minimal rectangle and color types. */
typedef struct { int x, y, w, h; } cyon_rect_t;
typedef struct { unsigned char r, g, b; } cyon_color_t;

/* Internal terminal state saved for restore. */
static struct termios cyon_saved_term;
static int cyon_term_saved = 0;

/* Enable raw mode for terminal input. Returns 0 on success. */
int cyon_tty_enable_raw(void) {
    if (cyon_term_saved) return 0;
    if (!isatty(STDIN_FILENO)) return ENOTTY;
    struct termios t;
    if (tcgetattr(STDIN_FILENO, &t) != 0) return errno ? errno : -1;
    cyon_saved_term = t;
    cyon_term_saved = 1;
    t.c_lflag &= ~(ICANON | ECHO);
    t.c_cc[VMIN] = 1;
    t.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &t) != 0) return errno ? errno : -1;
    return 0;
}

/* Restore terminal to saved state. */
int cyon_tty_restore(void) {
    if (!cyon_term_saved) return 0;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &cyon_saved_term) != 0) return errno ? errno : -1;
    cyon_term_saved = 0;
    return 0;
}

/* Query terminal size. Returns 0 on success. */
int cyon_tty_size(int *cols, int *rows) {
    if (!cols || !rows) return EINVAL;
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != 0) {
        return errno ? errno : -1;
    }
    *cols = (int)ws.ws_col;
    *rows = (int)ws.ws_row;
    return 0;
}

/* Clear screen and move cursor to home. */
int cyon_tty_clear(void) {
    const char *cmd = "\x1b[2J\x1b[H";
    if (write(STDOUT_FILENO, cmd, strlen(cmd)) < 0) return errno ? errno : -1;
    return 0;
}

/* Draw a bordered box using ASCII art. */
int cyon_draw_box(const cyon_rect_t *r, const char *title) {
    if (!r) return EINVAL;
    if (r->w < 2 || r->h < 2) return EINVAL;
    char buf[256];
    /* top border */
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH+%.*s+", r->y, r->x, r->w - 2, "................................................................................................................................................................................................................................................................");
    if (write(STDOUT_FILENO, buf, strlen(buf)) < 0) return errno ? errno : -1;
    /* title if provided */
    if (title && title[0]) {
        int tlen = (int)strlen(title);
        int copy = (tlen < r->w - 4) ? tlen : r->w - 4;
        snprintf(buf, sizeof(buf), "\x1b[%d;%dH| %.*s%*s |", r->y, r->x+1, copy, title, r->w - 4 - copy, "");
        if (write(STDOUT_FILENO, buf, strlen(buf)) < 0) return errno ? errno : -1;
    }
    /* middle */
    for (int i = 1; i < r->h - 1; ++i) {
        snprintf(buf, sizeof(buf), "\x1b[%d;%dH|%*s|", r->y + i, r->x + 1, r->w - 2, "");
        if (write(STDOUT_FILENO, buf, strlen(buf)) < 0) return errno ? errno : -1;
    }
    /* bottom */
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH+%.*s+", r->y + r->h - 1, r->x, r->w - 2, "................................................................................................................................................................................................................................................................");
    if (write(STDOUT_FILENO, buf, strlen(buf)) < 0) return errno ? errno : -1;
    return 0;
}

/* Simple input polling, non-blocking when raw mode enabled.
   Reads up to buf_len bytes into buf and returns bytes read or negative errno-like. */
ssize_t cyon_tty_read(char *buf, size_t buf_len) {
    if (!buf || buf_len == 0) return -1;
    ssize_t r = read(STDIN_FILENO, buf, buf_len);
    if (r < 0) return errno ? -errno : -1;
    return r;
}

/* Draw text at given terminal coordinates. */
int cyon_tty_draw_text(int x, int y, const char *text) {
    if (!text) return EINVAL;
    char seq[512];
    snprintf(seq, sizeof(seq), "\x1b[%d;%dH%s", y, x, text);
    if (write(STDOUT_FILENO, seq, strlen(seq)) < 0) return errno ? errno : -1;
    return 0;
}

/* Simple color to ANSI conversion, foreground only.
   Returns 0 on success. */
int cyon_tty_set_fg_color(cyon_color_t c) {
    /* Use 24-bit color sequence */
    char seq[64];
    snprintf(seq, sizeof(seq), "\x1b[38;2;%d;%d;%dm", c.r, c.g, c.b);
    if (write(STDOUT_FILENO, seq, strlen(seq)) < 0) return errno ? errno : -1;
    return 0;
}

/* Reset color attributes. */
int cyon_tty_reset_color(void) {
    if (write(STDOUT_FILENO, "\x1b[0m", 4) < 0) return errno ? errno : -1;
    return 0;
}

/* Very small dialog helper: prints message centered and waits for a key. */
int cyon_tty_dialog(const char *title, const char *message) {
    int cols = 80, rows = 24;
    cyon_tty_size(&cols, &rows);
    int w = cols * 2 / 3;
    int h = 7;
    int x = (cols - w) / 2 + 1;
    int y = (rows - h) / 2 + 1;
    cyon_rect_t r = { x, y, w, h };
    cyon_tty_clear();
    cyon_draw_box(&r, title ? title : "");
    cyon_tty_draw_text(x + 2, y + 2, message ? message : "");
    cyon_tty_draw_text(x + 2, y + h - 2, "Press any key to continue...");
    cyon_tty_enable_raw();
    char c;
    read(STDIN_FILENO, &c, 1);
    cyon_tty_restore();
    return 0;
}