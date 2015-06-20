/*
 * Copyright (c) 2014-2015 Łukasz S.
 * Distributed under the terms of GPL-2 License.
 */

/*
 * kernel/gui.c - graphical user interface structures and routines
 */

#include <kernel/kernel.h>

/* window object */
struct win {
    uint8_t active;
    uint16_t x, y, w, h;
    uint32_t *buf;
};

/* private functions */
static inline uint64_t win_blend_pixels(uint64_t b, uint64_t w);
static void win_draw(uint32_t *buf, struct win *win);

/* array of windows */
static struct win windows[8];

/* alpha-blend two pairs of pixels */
static inline uint64_t
win_blend_pixels(uint64_t b, uint64_t w)
{
    uint16_t a1, a2;

    a1 = (w & 0xFF00000000000000) >> 56;
    a2 = (w & 0x00000000FF000000) >> 24;

    b += (
        ((((w & 0x00FF000000000000) - (b & 0x00FF000000000000)) * a1) & 0xFF00000000000000) +
        ((((w & 0x0000FF0000000000) - (b & 0x0000FF0000000000)) * a1) & 0x00FF000000000000) +
        ((((w & 0x000000FF00000000) - (b & 0x000000FF00000000)) * a1) & 0x0000FF0000000000) +
        ((((w & 0x0000000000FF0000) - (b & 0x0000000000FF0000)) * a2) & 0x00000000FF000000) +
        ((((w & 0x000000000000FF00) - (b & 0x000000000000FF00)) * a2) & 0x0000000000FF0000) +
        ((((w & 0x00000000000000FF) - (b & 0x00000000000000FF)) * a2) & 0x000000000000FF00)
    ) >> 8;

    return b;
}
/* draw a window to the given buffer */
static void
win_draw(uint32_t *buf, struct win *win)
{
    uint16_t y, x;
    uint64_t *winp64, *bufp64;

    winp64 = (uint64_t *)win->buf;
    for (y = 0; y < win->h; ++y) {

        bufp64 = (uint64_t *)(buf + (win->y + y) * GUI_WIDTH + win->x);
        for (x = 0; x < (win->w >> 1); ++x, ++bufp64, ++winp64) {
            *bufp64 = win_blend_pixels(*bufp64, *winp64);
        }
    }
}

/* draw all active winows to the given buffer */
void
win_draw_all(uint32_t *buf)
{
    ARRAY_FOREACH(windows, i) {
        if (!windows[i].active)
            continue;
        win_draw(buf, &windows[i]);
    }
}

/* create a new window */
int
win_create(int x, int y, int w, int h, uint32_t *buf)
{
    struct win *win;

    win = ARRAY_TAKE(windows);
    if (!win) {
        return -1;
    }

    win->x = x;
    win->y = y;
    win->w = w;
    win->h = h;
    win->buf = buf;
    
    return (win - windows);
}

/* release a window */
int
win_close(int wd)
{
    struct win *win;

    win = &windows[wd];
    ARRAY_RELEASE(win);

    return 0;
}

/* initialize the array of windows */
void
win_init(void)
{
    ARRAY_INIT(windows);
}
