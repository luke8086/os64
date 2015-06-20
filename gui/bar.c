/*
 * Copyright (c) 2014-2015 Łukasz S.
 * Distributed under the terms of GPL-2 License.
 */

/*
 * kernel/bar.c - status bar task
 */

#include <kernel/kernel.h>
#include <gui/gui.h>

/* dimensions and padding */
enum {
    BAR_H_PAD   = 10,
    BAR_V_PAD   = 5,
    BAR_WIDTH   = GUI_WIDTH,
    BAR_HEIGHT  = FONT_HEIGHT + (2 * BAR_V_PAD),
};

/* foreground and background colors */
#define BAR_FG 0xffffffff
#define BAR_BG 0xc0000000

/* private functions */
static void bar_draw_bg(uint32_t *buf);
static void bar_draw_name(void);
static void bar_draw_time(void);
static void bar_draw_stat(void);

/* window buffers */
static uint32_t b1_buf[BAR_WIDTH * BAR_HEIGHT];
static uint32_t b2_buf[BAR_WIDTH * BAR_HEIGHT];

/* window descriptors */
static int b1_wd;
static int b2_wd;

/* fill the buffer with the background color */
static void
bar_draw_bg(uint32_t *buf)
{
    uint64_t *bufp;
    uint64_t bg;
    size_t i;

    bufp = (uint64_t *)buf;
    bg = ((uint64_t)BAR_BG << 32) | BAR_BG;

    for (i = 0; i < BAR_WIDTH * BAR_HEIGHT / 2; ++i) {
        *(bufp++) = bg;
    }
}

/* render system name and version */
static void
bar_draw_name(void)
{
    char name_buf[32];
    int name_pos;

    snprintf(name_buf, sizeof(name_buf), "%s %s", SYSTEM_NAME, SYSTEM_VERSION);
    name_pos = BAR_WIDTH * BAR_V_PAD + BAR_H_PAD;
    font_render_str(b1_buf + name_pos, BAR_WIDTH, name_buf, BAR_FG, BAR_BG);
}

/* render current date and time */
static void
bar_draw_time(void)
{
    char time_buf[20];
    struct time tm;
    int time_pos;
    int time_width;

    cmos_get_time(&tm);

    snprintf(time_buf, sizeof(time_buf), "%04d-%02d-%02d %02d:%02d:%02d",
             tm.year, tm.month, tm.day, tm.hour, tm.minute, tm.second);

    time_width = (sizeof(time_buf) - 1) * FONT_WIDTH;
    time_pos = BAR_WIDTH * BAR_V_PAD + BAR_WIDTH - BAR_H_PAD - time_width;
    
    font_render_str(b1_buf + time_pos, BAR_WIDTH, time_buf, BAR_FG, BAR_BG);
}

/* render memory and task info */
static void
bar_draw_stat(void)
{
    char stat_buf[40];
    int width, pos;
    size_t tasks, kheap;

    tasks = task_count();
    kheap = kheap_used() >> 10;

    snprintf(stat_buf, sizeof(stat_buf), "heap: %uK, tasks: %d", kheap, tasks);

    width = strlen(stat_buf) * FONT_WIDTH;
    pos = BAR_WIDTH * BAR_V_PAD + BAR_WIDTH - BAR_H_PAD - width;

    bar_draw_bg(b2_buf);
    font_render_str(b2_buf + pos, BAR_WIDTH, stat_buf, BAR_FG, BAR_BG);
}

/* main entry point of the task */
static void
bar_main(int argc, char **argv)
{
    b1_wd = win_create(0, 0, BAR_WIDTH, BAR_HEIGHT, b1_buf);
    kassert(b1_wd >= 0, "cannot create top bar window");

    b2_wd = win_create(0, GUI_HEIGHT - BAR_HEIGHT,
                              BAR_WIDTH, BAR_HEIGHT, b2_buf);
    kassert(b2_wd >= 0, "cannot create bottom bar window");

    bar_draw_bg(b1_buf);
    bar_draw_bg(b2_buf);

    while (1) {
        bar_draw_name();
        bar_draw_time();
        bar_draw_stat();
        gui_redraw();
        task_sleep(500);
    }
    
    // NOTREACHED
}

/* spawn the background task */
void
bar_init(void)
{
    task_spawn((uintptr_t)bar_main, 0, 0);
}
