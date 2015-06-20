/*
 * Copyright (c) 2014-2015 Łukasz S.
 * Distributed under the terms of GPL-2 License.
 */

#ifndef _GUI_GUI_H_
#define _GUI_GUI_H_

#include <libc/types.h>

/* font dimensions */
enum {
    FONT_WIDTH      = 8,
    FONT_HEIGHT     = 16,
    FONT_LENGTH     = 256,
};

/* gui/bar.c */
void bar_init(void);

/* gui/fbcon.c */
void fbcon_flush(uint16_t *buf, int cols, int rows);
void fbcon_init(void);

/* gui/font.c */
void font_render_char(uint32_t *buf, int linew, uint8_t ch, uint32_t fg, uint32_t bg);
void font_render_str(uint32_t *buf, int linew, char *s, uint32_t fg, uint32_t bg);
void font_init(void);

/* gui/gui.c */
void gui_set_bg(const char *path);
void gui_redraw(void);
void gui_init(void);

/* gui/win.c */
void win_draw_all(uint32_t *buf);
int win_create(int x, int y, int w, int h, uint32_t *buf);
int win_close(int wd);
void win_init(void);

#endif // _GUI_GUI_H_
