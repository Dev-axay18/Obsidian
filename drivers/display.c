/**
 * 💠 Obsidian OS - Display Driver
 * Text mode display driver for VGA/EGA compatible displays
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

// VGA display constants
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_BUFFER 0xB8000

// Color constants
#define VGA_BLACK 0
#define VGA_BLUE 1
#define VGA_GREEN 2
#define VGA_CYAN 3
#define VGA_RED 4
#define VGA_MAGENTA 5
#define VGA_BROWN 6
#define VGA_LIGHT_GREY 7
#define VGA_DARK_GREY 8
#define VGA_LIGHT_BLUE 9
#define VGA_LIGHT_GREEN 10
#define VGA_LIGHT_CYAN 11
#define VGA_LIGHT_RED 12
#define VGA_LIGHT_MAGENTA 13
#define VGA_LIGHT_BROWN 14
#define VGA_WHITE 15

// Display state
static uint16_t* vga_buffer = (uint16_t*)VGA_BUFFER;
static uint8_t current_color = VGA_LIGHT_GREY;
static uint8_t current_bg_color = VGA_BLACK;
static int cursor_x = 0;
static int cursor_y = 0;

/**
 * Initialize display driver
 */
void display_init(void) {
    // Clear screen
    display_clear();
    
    // Set initial cursor position
    cursor_x = 0;
    cursor_y = 0;
    
    // Set default colors
    current_color = VGA_LIGHT_GREY;
    current_bg_color = VGA_BLACK;
}

/**
 * Set text color
 */
void display_set_color(uint8_t color) {
    current_color = color;
}

/**
 * Set background color
 */
void display_set_bg_color(uint8_t color) {
    current_bg_color = color;
}

/**
 * Get color attribute
 */
static uint8_t get_color_attr(void) {
    return current_color | (current_bg_color << 4);
}

/**
 * Put character at position
 */
void display_putchar_at(char c, int x, int y) {
    if (x < 0 || x >= VGA_WIDTH || y < 0 || y >= VGA_HEIGHT) {
        return;
    }
    
    uint16_t attribute = get_color_attr() << 8;
    uint16_t character = c | attribute;
    
    vga_buffer[y * VGA_WIDTH + x] = character;
}

/**
 * Put character at current cursor position
 */
void display_putchar(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
        if (cursor_y >= VGA_HEIGHT) {
            display_scroll();
            cursor_y = VGA_HEIGHT - 1;
        }
        return;
    }
    
    if (c == '\r') {
        cursor_x = 0;
        return;
    }
    
    if (c == '\t') {
        cursor_x = (cursor_x + 4) & ~3;  // Align to 4-character boundary
        if (cursor_x >= VGA_WIDTH) {
            cursor_x = 0;
            cursor_y++;
            if (cursor_y >= VGA_HEIGHT) {
                display_scroll();
                cursor_y = VGA_HEIGHT - 1;
            }
        }
        return;
    }
    
    display_putchar_at(c, cursor_x, cursor_y);
    cursor_x++;
    
    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
        if (cursor_y >= VGA_HEIGHT) {
            display_scroll();
            cursor_y = VGA_HEIGHT - 1;
        }
    }
}

/**
 * Print string
 */
void display_print(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        display_putchar(str[i]);
    }
}

/**
 * Print formatted string (simple implementation)
 */
void display_printf(const char* format, ...) {
    // Simple implementation - just print the format string
    // In a real implementation, this would handle format specifiers
    display_print(format);
}

/**
 * Clear screen
 */
void display_clear(void) {
    uint16_t blank = ' ' | (get_color_attr() << 8);
    
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = blank;
    }
    
    cursor_x = 0;
    cursor_y = 0;
}

/**
 * Clear line
 */
void display_clear_line(int y) {
    if (y < 0 || y >= VGA_HEIGHT) {
        return;
    }
    
    uint16_t blank = ' ' | (get_color_attr() << 8);
    
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga_buffer[y * VGA_WIDTH + x] = blank;
    }
}

/**
 * Scroll screen up
 */
void display_scroll(void) {
    // Move all lines up by one
    for (int y = 0; y < VGA_HEIGHT - 1; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[y * VGA_WIDTH + x] = vga_buffer[(y + 1) * VGA_WIDTH + x];
        }
    }
    
    // Clear bottom line
    display_clear_line(VGA_HEIGHT - 1);
}

/**
 * Set cursor position
 */
void display_set_cursor(int x, int y) {
    if (x < 0 || x >= VGA_WIDTH || y < 0 || y >= VGA_HEIGHT) {
        return;
    }
    
    cursor_x = x;
    cursor_y = y;
}

/**
 * Get cursor position
 */
void display_get_cursor(int* x, int* y) {
    if (x) *x = cursor_x;
    if (y) *y = cursor_y;
}

/**
 * Hide cursor
 */
void display_hide_cursor(void) {
    // Send command to VGA controller to hide cursor
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);
}

/**
 * Show cursor
 */
void display_show_cursor(void) {
    // Send command to VGA controller to show cursor
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x0E);
    outb(0x3D4, 0x0B);
    outb(0x3D5, 0x0F);
}

/**
 * Update cursor position in hardware
 */
void display_update_cursor(void) {
    uint16_t pos = cursor_y * VGA_WIDTH + cursor_x;
    
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

/**
 * Draw box
 */
void display_draw_box(int x, int y, int width, int height, char border_char) {
    if (x < 0 || y < 0 || x + width > VGA_WIDTH || y + height > VGA_HEIGHT) {
        return;
    }
    
    // Draw top and bottom borders
    for (int i = 0; i < width; i++) {
        display_putchar_at(border_char, x + i, y);
        display_putchar_at(border_char, x + i, y + height - 1);
    }
    
    // Draw left and right borders
    for (int i = 1; i < height - 1; i++) {
        display_putchar_at(border_char, x, y + i);
        display_putchar_at(border_char, x + width - 1, y + i);
    }
}

/**
 * Fill rectangle
 */
void display_fill_rect(int x, int y, int width, int height, char fill_char) {
    if (x < 0 || y < 0 || x + width > VGA_WIDTH || y + height > VGA_HEIGHT) {
        return;
    }
    
    for (int dy = 0; dy < height; dy++) {
        for (int dx = 0; dx < width; dx++) {
            display_putchar_at(fill_char, x + dx, y + dy);
        }
    }
}

/**
 * Output byte to port
 */
static void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
} 