/*
 * Copyright (c) 2014-2015 Łukasz S.
 * Distributed under the terms of GPL-2 License.
 */

/*
 * kernel/kbd.c - PS/2 (8042) keyboard driver
 */

#include <kernel/kernel.h>

/* character map for the XT scancode set */
static unsigned char kbd_map_default[] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't','y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';','\'', '`', 0, '\\', 'z',
    'x', 'c', 'v', 'b', 'n','m', ',', '.', '/', 0,'*', 0,' ', 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 254, 0, '-', 252, 0, 253, '+', 0, 255, 0, 0, 0, 0, 0, 0, 0,
    0, 0,
};

/* character map for the XT scancode set with shift */
static unsigned char kbd_map_shift[] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T','Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0,
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':','"', '~', 0, '|', 'Z',
    'X', 'C', 'V', 'B', 'N','M', '<', '>', '?', 0,'*', 0,' ', 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 254, 0, '-', 252, 0, 253, '+', 0, 255, 0, 0, 0, 0, 0, 0, 0,
    0, 0,
};

/* i/o port address */
enum {
    KBD_DATA_PORT = 0x60,
};

/* keyboard buffer */
static struct kbd_buf {
    uint16_t buf[16];
    uint8_t write_ptr;
    uint8_t read_ptr;
    uint8_t count;
    uint8_t size;
} kbd_buf;

/* private  functions */
static void kbd_handler(uint8_t intno, struct intr_stack *intr_stack, struct regs *regs);
static int kbd_buf_append(uint16_t key);
static int kbd_buf_pop(uint16_t *key);

/* handle keyboard interrupt */
static void
kbd_handler(uint8_t intno, struct intr_stack *intr_stack, struct regs *regs)
{
    static uint8_t shift = 0;
    unsigned char *map;
    uint16_t key;
    uint8_t code;

    code = cpu_inb(KBD_DATA_PORT);

    if (code == 0xb6 || code == 0xaa) {
        shift = 0; 
        return;
    }

    if (code & 0x80) {
        return;
    }

    if (code == 0x36 || code == 0x2a) {
        shift = 1;
        return;
    }

    map = shift ? kbd_map_shift : kbd_map_default;

    key = ((uint16_t)code << 8) | map[code];

    (void)kbd_buf_append(key);
}

/*
 * append a single key to the buffer
 * returns 0 on success and 1 on full buffer
 */
static int
kbd_buf_append(uint16_t key)
{
    if (kbd_buf.count >= kbd_buf.size) {
        return 1;
    }

    kbd_buf.buf[kbd_buf.write_ptr] = key;
    kbd_buf.count += 1;
    kbd_buf.write_ptr = (kbd_buf.write_ptr + 1) % kbd_buf.size;

    return 0;
}

/*
 * take single key from the buffer, not thread-safe.
 * returns 0 on success and 1 on empty buffer.
 */
static int
kbd_buf_pop(uint16_t *key)
{
    if (kbd_buf.count == 0) {
        return 1;
    }

    *key = kbd_buf.buf[kbd_buf.read_ptr];
    kbd_buf.count -= 1;
    kbd_buf.read_ptr = (kbd_buf.read_ptr + 1) % kbd_buf.size;

    return 0;
}

/*
 * public, thread-safe interface to read a single key from the buffer
 * returns 0 on success and 1 on empty buffer.
 */
int
kbd_read(uint16_t *key)
{
    int ret;
    uint64_t flags;

    flags = cpu_get_flags();
    cpu_cli();
    ret = kbd_buf_pop(key);
    cpu_set_flags(flags);

    return ret;
}

/* initialize the keyboard buffer and set the interrupt handler */
void
kbd_init(void)
{
    kbd_buf.write_ptr = 0;
    kbd_buf.read_ptr = 0;
    kbd_buf.count = 0;
    kbd_buf.size = sizeof(kbd_buf.buf) / sizeof(kbd_buf.buf[0]);

    intr_set_handler(0x21, kbd_handler);
}
