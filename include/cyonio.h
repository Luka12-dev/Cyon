#ifndef CYONIO_H
#define CYONIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cyonlib.h"

/* Terminal / TTY helpers */
CYON_API int cyon_tty_enable_raw(void);
CYON_API int cyon_tty_restore(void);
CYON_API int cyon_tty_size(int *cols, int *rows);
CYON_API int cyon_tty_clear(void);
CYON_API int cyon_draw_box(const cyon_rect_t *r, const char *title);
CYON_API ssize_t cyon_tty_read(char *buf, size_t buf_len);
CYON_API int cyon_tty_draw_text(int x, int y, const char *text);
CYON_API int cyon_tty_set_fg_color(cyon_color_t c);
CYON_API int cyon_tty_reset_color(void);
CYON_API int cyon_tty_dialog(const char *title, const char *message);

#ifdef __cplusplus
}
#endif

#endif /* CYONIO_H */