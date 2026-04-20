#include "mouse.h"
#include "isr.h"
#include "io.h"

namespace mouse {

static int32_t mouse_x = 1280 / 2;
static int32_t mouse_y = 800 / 2;
static bool left_clicked = false;
static bool right_clicked = false;
static bool middle_clicked = false;

static uint8_t mouse_cycle = 0;
static int8_t mouse_byte[3];

static void mouse_wait(uint8_t a_type) {
    uint32_t timeout = 100000;
    if (a_type == 0) {
        while (timeout--) {
            if ((inb(0x64) & 1) == 1) return;
        }
    } else {
        while (timeout--) {
            if ((inb(0x64) & 2) == 0) return;
        }
    }
}

static void mouse_write(uint8_t a_write) {
    mouse_wait(1);
    outb(0x64, 0xD4);
    mouse_wait(1);
    outb(0x60, a_write);
}

static uint8_t mouse_read() {
    mouse_wait(0);
    return inb(0x60);
}

static void callback(InterruptFrame *) {
    uint8_t data = inb(0x60);
    mouse_byte[mouse_cycle++] = data;

    if (mouse_cycle == 1) {
        // Bit 3 of the first byte must be 1. If not, we are out of sync.
        if (!(data & 0x08)) {
            mouse_cycle = 0;
            return;
        }
    }

    if (mouse_cycle == 3) {
        mouse_cycle = 0;

        uint8_t state = mouse_byte[0];
        int x_mov = mouse_byte[1];
        int y_mov = mouse_byte[2];

        // Handle sign extension
        if (state & (1 << 4)) x_mov |= 0xFFFFFF00;
        if (state & (1 << 5)) y_mov |= 0xFFFFFF00;

        // Update coords
        mouse_x += x_mov;
        mouse_y -= y_mov; // Y is flipped in PS/2

        // Bounds checking
        if (mouse_x < 0) mouse_x = 0;
        if (mouse_x > 1279) mouse_x = 1279;
        if (mouse_y < 0) mouse_y = 0;
        if (mouse_y > 799) mouse_y = 799;

        left_clicked = state & (1 << 0);
        right_clicked = state & (1 << 1);
        middle_clicked = state & (1 << 2);
    }
}

void init() {
    mouse_wait(1);
    outb(0x64, 0xA8); // Enable auxiliary device

    mouse_wait(1);
    outb(0x64, 0x20); // Get compaq status
    mouse_wait(0);
    uint8_t status = inb(0x60);
    status |= 2; // Enable IRQ12
    status &= ~0x20; // Disable mouse clock
    mouse_wait(1);
    outb(0x64, 0x60); // Set compaq status
    mouse_wait(1);
    outb(0x60, status);

    mouse_write(0xF6); // Use default settings
    mouse_read(); // Acknowledge

    mouse_write(0xF4); // Enable packet streaming
    mouse_read(); // Acknowledge

    isr::register_handler(44, callback);

    // Unmask IRQ2 in Master PIC (Cascade for Slave)
    uint8_t master_mask = inb(0x21);
    master_mask &= ~(1 << 2);
    outb(0x21, master_mask);

    // Unmask IRQ12 in Slave PIC
    uint8_t slave_mask = inb(0xA1);
    slave_mask &= ~(1 << 4);
    outb(0xA1, slave_mask);
}

int32_t get_x() { return mouse_x; }
int32_t get_y() { return mouse_y; }
bool is_left_clicked() { return left_clicked; }
bool is_right_clicked() { return right_clicked; }
bool is_middle_clicked() { return middle_clicked; }

}
