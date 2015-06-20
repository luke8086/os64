/*
 * Copyright (c) 2014-2015 Łukasz S.
 * Distributed under the terms of GPL-2 License.
 */

/*
 * kernel/font.c - character rendering routines
 */

#include <kernel/kernel.h>
#include <gui/gui.h>

#define FONT_PATH "/data/vga_8x16.fnt"

/* private functions */
static inline void font_render_space(uint32_t *buf, int linew, uint32_t bg);
static void font_load(void);

/* private data */
static uint8_t font_buffer[FONT_HEIGHT * FONT_LENGTH];

/* load font from file */
static void
font_load(void)
{
    ssize_t size;
    int fd;

    fd = vfs_open(FONT_PATH);
    kassert(fd >= 0, "cannot open font file");

    size = file_read(fd, font_buffer, sizeof(font_buffer));
    kassert(size == sizeof(font_buffer), "error reading font file");

    (void)file_close(fd);
}

/* render a single blank character to the specified buffer */
static inline void
font_render_space(uint32_t *buf, int linew, uint32_t bg)
{
    uint64_t *charp;
    uint64_t bg64;
    int i, j;

    bg64 = bg | ((uint64_t)bg << 32);
    charp = (uint64_t *)buf;

    for (j = 0; j < FONT_HEIGHT; ++j) {
        for (i = 0; i < FONT_WIDTH >> 1; ++i) {
            *charp++ = bg64;
        }
        charp += (linew - FONT_WIDTH) >> 1;
    }
}

/* render a single character to the specified buffer */
void
font_render_char(uint32_t *buf, int linew, uint8_t ch, uint32_t fg, uint32_t bg)
{
    uint8_t *glyph;
    uint8_t active;
    int i, j;

    if (!ch || ch == ' ') {
        font_render_space(buf, linew, bg);
        return;
    }

    glyph = font_buffer + (ch * FONT_HEIGHT);

    for (j = 0; j < FONT_HEIGHT; ++j) {
        for (i = 0; i < FONT_WIDTH; ++i) {
            active = (glyph[j] & (1 << (7 - i)));
            *buf++ = fg * !!active + bg * !active;
        }
        buf += linew - FONT_WIDTH;
    }
}

/* render a string to the specified buffer  */
void
font_render_str(uint32_t *buf, int linew, char *s, uint32_t fg, uint32_t bg)
{
    while (*s) {
        font_render_char(buf, linew, *s, fg, bg);
        s++;
        buf += FONT_WIDTH;
    }
}

/* load font data from file */
void
font_init(void)
{
    font_load();
}
