/*
 * Copyright (c) 2014-2015 Łukasz S.
 * Distributed under the terms of GPL-2 License.
 */

/*
 * gui/gui.c - basic graphical user interface routines
 */

#include <kernel/kernel.h>
#include <gui/gui.h>

/* filesystem paths */
#define GUI_BG_FILE "/data/bg-green.pix"

/* private functions */
static void gui_draw_buf(void);

/* private data */
static uint32_t gui_buffer[GUI_WIDTH * GUI_HEIGHT];
static uint32_t gui_bg_buffer[GUI_WIDTH * GUI_HEIGHT];

/* set the background image */
void
gui_set_bg(const char *path)
{
    int fd;
    uint16_t w, h;

    fd = vfs_open(path);
    if (fd < 0)
        return;

    w = h = 0;
    (void)file_read(fd, &w, sizeof(w));
    (void)file_read(fd, &h, sizeof(h));
    if (w != GUI_WIDTH || h != GUI_HEIGHT)
        return;

    (void)file_read(fd, gui_bg_buffer, sizeof(gui_bg_buffer));
    (void)file_close(fd);

    gui_redraw();
}

/* draw the gui buffer to the video memory */
static void
gui_draw_buf(void)
{
    uint32_t *fb = (uint32_t*)vbe_gfx_addr();
    uint32_t *gb = gui_buffer;

    if (vbe_bpp() == 4) {
        memcpy(fb, gui_buffer, sizeof(gui_buffer));
        return;
    }

    if (vbe_bpp() != 3) {
        return;
    }

    for (int y = 0; y < GUI_HEIGHT; ++y) {
        for (int x = 0; x < GUI_WIDTH; ++x) {
            *fb = *gb | (*fb & 0xFF000000);
            fb = (uint32_t*)((uintptr_t)fb + 3);
            gb++;
        }
    }
}

/* redraw background and all active windows to the screen */
void
gui_redraw(void)
{
    memcpy(gui_buffer, gui_bg_buffer, sizeof(gui_buffer));
    win_draw_all(gui_buffer);
    gui_draw_buf();
}

/* load the background */
void
gui_init(void)
{
    gui_set_bg(GUI_BG_FILE);
}
