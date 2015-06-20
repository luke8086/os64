/*
 * Copyright (c) 2014-2015 Łukasz S.
 * Distributed under the terms of GPL-2 License.
 */

/*
 * kernel/fbcon.c - frame buffer console
 */

#include <kernel/kernel.h>
#include <gui/gui.h>

/* window dimensions */
enum {
    FBCON_WIDTH  = VT_COLS * FONT_WIDTH,
    FBCON_HEIGHT = VT_ROWS * FONT_HEIGHT,
};

/* color palette */
#define FBCON_RGB(r, g, b) ((r << 16) | (g << 8) | (b << 0))

static uint32_t fbcon_palette[] = {
    FBCON_RGB(0x00, 0x00, 0x00),  // black
    FBCON_RGB(0x00, 0x00, 0xC0),  // blue
    FBCON_RGB(0x00, 0xC0, 0x00),  // green
    FBCON_RGB(0x00, 0xC0, 0xC0),  // cyan
    FBCON_RGB(0xC0, 0x00, 0x00),  // red
    FBCON_RGB(0xC0, 0x00, 0xC0),  // magenta
    FBCON_RGB(0xC0, 0x80, 0x00),  // brown
    FBCON_RGB(0xC0, 0xC0, 0xC0),  // light gray
    FBCON_RGB(0x80, 0x80, 0x80),  // dark gray
    FBCON_RGB(0x00, 0x00, 0xFF),  // light blue
    FBCON_RGB(0x00, 0xFF, 0x00),  // light green            
    FBCON_RGB(0x00, 0xFF, 0xFF),  // light cyan
    FBCON_RGB(0xFF, 0x00, 0x00),  // light red
    FBCON_RGB(0xFF, 0x00, 0xFF),  // light magenta
    FBCON_RGB(0xFF, 0xFF, 0x00),  // yellow
    FBCON_RGB(0xFF, 0xFF, 0xFF),  // white
};

/* window buffer and descriptor */
static uint32_t fbcon_buf[FBCON_WIDTH * FBCON_HEIGHT];
static int fbcon_wd;

/* redraw the terminal buffer */
void
fbcon_flush(uint16_t *tbuf, int cols, int rows)
{
    int i, j, linew;
    uint8_t ch;
    uint32_t fg, bg;
    uint32_t *gbufp;
    uint16_t *tbufp;
    int fg_idx, bg_idx;

    memset(fbcon_buf, 0, sizeof(fbcon_buf));

    linew = FONT_WIDTH * VT_COLS;
    tbufp = tbuf;
    gbufp = fbcon_buf;

    for (j = 0; j < rows; ++j) {
        for (i = 0; i < cols; ++i) {
            ch = *tbufp & 0xFF;
            fg_idx = (*tbufp >> 8) & 0x0F;
            bg_idx = (*tbufp >> 8) & 0xF0;

            fg = fbcon_palette[fg_idx] | 0xFF000000;
            bg = fbcon_palette[bg_idx] | 0xC0000000;

            font_render_char(gbufp, linew, ch, fg, bg);

            tbufp++;
            gbufp += FONT_WIDTH;
        }
        gbufp += linew * (FONT_HEIGHT - 1);
    }

    gui_redraw();
}

/* create window and connect to the vt driver */
void
fbcon_init(void)
{
    int x, y;

    x = (GUI_WIDTH - FBCON_WIDTH) >> 1;
    y = (GUI_HEIGHT - FBCON_HEIGHT) >> 1;

    fbcon_wd = win_create(x, y, FBCON_WIDTH, FBCON_HEIGHT, fbcon_buf);
    kassert(fbcon_wd >= 0, "cannot create fbcon window");

    vt_set_flush_cb(fbcon_flush);
}
