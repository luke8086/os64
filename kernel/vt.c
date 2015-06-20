/*
 * Copyright (c) 2014-2015 Łukasz S.
 * Distributed under the terms of GPL-2 License.
 */

/*
 * kernel/vt.c - minimal video terminal
 */

#include <kernel/kernel.h>

/* video memory address */
enum {
    VT_VIDMEM = 0xB8000,
};

/* current input state */
enum {
    VT_ST_CHAR = 0,     // regular character
    VT_ST_CMD = 1,      // command number
    VT_ST_PARAM = 2,    // command parameter
};

/* control commands */
enum {
    VT_CMD_CLR = 1,
    VT_CMD_GOTOX = 2,
    VT_CMD_GOTOY = 3,
    VT_CMD_CHATTR = 4,
};

/* convenience macros */
#define VT_BUF_SIZE (VT_COLS * VT_ROWS)
#define VT_CR_POS   (cr_y * VT_COLS + cr_x)

/* private data */
static vt_flush_cb flush_cb = 0;
static uint16_t buffer[VT_BUF_SIZE];
static uint16_t *vidmem = (uint16_t*)VT_VIDMEM;
static uint8_t cr_x = 0;
static uint8_t cr_y = 0;
static uint8_t cr_attr = 0x0F;

/* private functions */
static void vt_goto(uint8_t x, uint8_t y);
static void vt_clr(void);
static void vt_dispatch(uint8_t cmd, uint8_t param);
static void vt_advance(void);
static void vt_scroll(void);
static void vt_putc(unsigned char chr);
static void vt_flush(void);

/* move cursor to the specified position */
static void
vt_goto(uint8_t x, uint8_t y)
{
    cr_x = x;
    cr_y = y;
    crtc_cursor_set(VT_CR_POS);
}

/* clear the internal buffer */
static void
vt_clr(void)
{
    uint16_t word = (cr_attr << 8) | ' ';
    for (size_t i = 0; i < VT_BUF_SIZE; ++i) {
        buffer[i] = word;
    }
    vt_goto(0, 0);
}

/* dispatch command */
static void
vt_dispatch(uint8_t cmd, uint8_t param)
{
    switch(cmd) {

    case VT_CMD_CLR:
        vt_clr();
        break;

    case VT_CMD_GOTOX:
        vt_goto(param, cr_y);
        break;

    case VT_CMD_GOTOY:
        vt_goto(cr_x, param);
        break;

    case VT_CMD_CHATTR:
        cr_attr = param;
        break;
    }
}

/* advance cursor by one character */
static void
vt_advance(void)
{
    cr_x += 1;

    if (cr_x >= VT_COLS) {
        cr_y += cr_x / VT_COLS;
        cr_x = cr_x % VT_COLS;
    }

    crtc_cursor_set(VT_CR_POS);
}

/* scroll buffer down to the cursor position */
static void
vt_scroll(void)
{
    while (cr_y >= VT_ROWS) {

        for (uint8_t row = 0; row < VT_ROWS - 1; ++row) {
            void *dst = buffer + row * VT_COLS;
            void *src = buffer + (row + 1) * VT_COLS;
            size_t count = sizeof(buffer[0]) * VT_COLS;
            memcpy(dst, src, count);
        }

        for (int16_t i = 0; i < VT_COLS; ++i) {
            buffer[VT_COLS * VT_ROWS - 1 - i] = cr_attr << 8 | ' ';
        }

        cr_y--;
    }
}

/* handle one character of input */
static void
vt_putc(unsigned char chr)
{
    static uint8_t state = VT_ST_CHAR;
    static uint8_t command = 0;
    static uint8_t param = 0;

    switch (state) {

    case VT_ST_CHAR:
        if (chr == '\n') {
            vt_goto(0, cr_y + 1);
            vt_scroll();
        } else if (chr == '\t') {
            cr_x += 8;
            cr_x &= ~7;
            cr_x -= 1;
            vt_advance();
        } else if (chr == '\b') {
            if (cr_x > 0) {
                --cr_x;
                buffer[VT_CR_POS] = (cr_attr << 8) | ' ';
                crtc_cursor_set(VT_CR_POS);
            }
        } else if (chr == '\033') {
            state = VT_ST_CMD;
        } else if (chr >= 32) {
            vt_scroll();
            buffer[VT_CR_POS] = (cr_attr << 8) | chr;
            vt_advance();
        }
        break;

    case VT_ST_CMD:
        command = chr;
        state = VT_ST_PARAM;
        break;

    case VT_ST_PARAM:
        param = chr;
        vt_dispatch(command, param);
        state = VT_ST_CHAR;
        break;
    }
}

/* flush internal buffer to the video memory */
static void
vt_flush(void)
{
    memcpy(vidmem, buffer, VT_COLS * VT_ROWS * 2);
    if (flush_cb) {
        flush_cb(buffer, VT_COLS, VT_ROWS);
    }
}

/* handle n characters from the given buffer */
size_t
vt_write(const char *buf, size_t n)
{
    for (size_t i = 0; i < n; ++i) {
        vt_putc((unsigned char)buf[i]);
    }
    vt_flush();

    return n;
}

/* set flush callback */
void
vt_set_flush_cb(vt_flush_cb cb)
{
    flush_cb = cb;
}

/* initialize terminal */
void
vt_init(void)
{
    // read cursor position
    uint16_t pos = crtc_cursor_get();
    cr_x = pos % VT_COLS;
    cr_y = pos / VT_COLS;

    // copy existing video memory to our buffer
    for (int i = 0; i < (VT_COLS * VT_ROWS); ++i) {
        buffer[i] = vidmem[i] != 0xFFFF ? vidmem[i] : 0;
    }
}
