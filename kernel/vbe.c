/*
 * Copyright (c) 2014-2015 Łukasz S.
 * Distributed under the terms of GPL-2 License.
 */

/*
 * kernel/vbe.c - vesa bios extensions driver
 */

#include <kernel/kernel.h>

/* vbe 2.0 mode info block */
struct vbe_mode_info {
    uint16_t mode_attributes;
    uint8_t win_a_attributes;
    uint8_t win_b_attributes;
    uint16_t win_granularity;
    uint16_t win_size;
    uint16_t win_a_segment;
    uint16_t win_b_segment;
    uint32_t win_func_ptr;
    uint16_t bytes_per_scan_line;

    uint16_t x_res;
    uint16_t y_res;
    uint8_t x_char_size;
    uint8_t y_char_size;
    uint8_t number_of_planes;
    uint8_t bits_per_pixel;
    uint8_t number_of_banks;
    uint8_t memory_model;
    uint8_t bank_size;
    uint8_t number_of_image_pages;
    uint8_t reserved_0;
 
    uint8_t red_mask_size;
    uint8_t red_field_position;
    uint8_t green_mask_size;
    uint8_t green_field_position;
    uint8_t blue_mask_size;
    uint8_t blue_field_position;
    uint8_t rsvd_mask_size;
    uint8_t rsvd_field_position;
    uint8_t direct_color_mode_info;
 
    uint32_t phys_base_ptr;
    uint32_t reserved_1;
    uint16_t reserved_2;
} __attribute__ ((packed));

/* linear frame buffer address */
static uint8_t *vbe_gfx_addr_p = 0;

/* bytes per pixel */
static uint8_t vbe_bpp_p = 0;

/* return 0 for text mode, 1 for graphics mode */
int
vbe_gfx_mode(void)
{
   return !!vbe_gfx_addr_p; 
}

/* return the linear frame buffer address */
uint8_t *
vbe_gfx_addr(void)
{
    return vbe_gfx_addr_p;
}

/* return bytes per pixel */
uint8_t
vbe_bpp(void)
{
    return vbe_bpp_p;
}

/* initialize video mode */
void
vbe_init(void)
{
    // get vbe_mode_info structure from multiboot
    uintptr_t info_ptr = mboot_vbe_mode_info_ptr();
    struct vbe_mode_info *info = (struct vbe_mode_info *)info_ptr;
    kassert(info, "vbe mode info not set");

    // return if not in gfx mode
    if (!info->phys_base_ptr)
        return;

    printk(KERN_INFO, "video mode:\n");
    printk(KERN_INFO, "  resolution:  %dx%dx%d\n",
           info->x_res, info->y_res, info->bits_per_pixel);
    printk(KERN_INFO, "  pitch: %d, memory_model: %d\n",
           info->bytes_per_scan_line, info->memory_model);

    // make sure current video mode is the supported one
    kassert(info->x_res == GUI_WIDTH, "unsupported video mode");
    kassert(info->y_res == GUI_HEIGHT, "unsupported video mode");

    kassert((info->bytes_per_scan_line == GUI_WIDTH * 4) ||
            (info->bytes_per_scan_line == GUI_WIDTH * 3),
            "unsupported video mode (pitch)");
    kassert(info->bits_per_pixel == 24 || info->bits_per_pixel == 32,
            "unsupported video mode (bpp)");
    kassert(info->memory_model == 0x06,
            "unsupported video mode (memory model)");

    // page map framebuffer address, calculate virtual address
    uintptr_t base_ptr = info->phys_base_ptr;
    uintptr_t base_ofs = base_ptr % MEM_PAGE_SIZE;
    uintptr_t base_page = base_ptr - base_ofs;
    ptt_map(VID_PAGE_ADDR, base_page, 1, 1);

    // store private data
    vbe_gfx_addr_p = (uint8_t*)(VID_PAGE_ADDR + base_ofs);
    vbe_bpp_p = info->bits_per_pixel / 8;
}
