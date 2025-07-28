/**
 * 💠 Obsidian OS - Keyboard Driver
 * PS/2 keyboard driver with event handling
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

// Keyboard constants
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_COMMAND_PORT 0x64
#define KEYBOARD_BUFFER_SIZE 256

// Key codes
#define KEY_ESCAPE 0x01
#define KEY_BACKSPACE 0x0E
#define KEY_TAB 0x0F
#define KEY_ENTER 0x1C
#define KEY_CTRL 0x1D
#define KEY_SHIFT_LEFT 0x2A
#define KEY_SHIFT_RIGHT 0x36
#define KEY_ALT 0x38
#define KEY_SPACE 0x39
#define KEY_CAPS_LOCK 0x3A
#define KEY_F1 0x3B
#define KEY_F2 0x3C
#define KEY_F3 0x3D
#define KEY_F4 0x3E
#define KEY_F5 0x3F
#define KEY_F6 0x40
#define KEY_F7 0x41
#define KEY_F8 0x42
#define KEY_F9 0x43
#define KEY_F10 0x44
#define KEY_F11 0x57
#define KEY_F12 0x58
#define KEY_NUM_LOCK 0x45
#define KEY_SCROLL_LOCK 0x46
#define KEY_HOME 0x47
#define KEY_UP 0x48
#define KEY_PAGE_UP 0x49
#define KEY_LEFT 0x4B
#define KEY_RIGHT 0x4D
#define KEY_END 0x4F
#define KEY_DOWN 0x50
#define KEY_PAGE_DOWN 0x51
#define KEY_INSERT 0x52
#define KEY_DELETE 0x53

// Keyboard state
typedef struct {
    bool shift_pressed;
    bool ctrl_pressed;
    bool alt_pressed;
    bool caps_lock;
    bool num_lock;
    bool scroll_lock;
} keyboard_state_t;

// Keyboard event structure
typedef struct {
    uint8_t key_code;
    bool pressed;
    char ascii_char;
    bool is_special;
} keyboard_event_t;

// Global variables
static keyboard_state_t keyboard_state = {0};
static keyboard_event_t event_buffer[KEYBOARD_BUFFER_SIZE];
static int buffer_head = 0;
static int buffer_tail = 0;
static bool keyboard_initialized = false;

// ASCII conversion table (without shift)
static const char ascii_table[128] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0, 0,
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0, 'a', 's',
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z', 'x', 'c', 'v',
    'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-', '4', '5', '6', '+', '1',
    '2', '3', '0', '.', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// ASCII conversion table (with shift)
static const char ascii_table_shift[128] = {
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0, 0,
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0, 'A', 'S',
    'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|', 'Z', 'X', 'C', 'V',
    'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-', '4', '5', '6', '+', '1',
    '2', '3', '0', '.', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/**
 * Initialize keyboard driver
 */
void keyboard_init(void) {
    // Clear keyboard state
    memset(&keyboard_state, 0, sizeof(keyboard_state));
    
    // Clear event buffer
    memset(event_buffer, 0, sizeof(event_buffer));
    buffer_head = buffer_tail = 0;
    
    // Send keyboard reset command
    keyboard_send_command(0xAD);  // Disable keyboard
    keyboard_send_command(0xAE);  // Enable keyboard
    
    // Set keyboard typematic rate
    keyboard_send_command(0xF3);  // Set typematic rate
    keyboard_send_data(0x00);     // Fastest rate
    
    keyboard_initialized = true;
}

/**
 * Send command to keyboard controller
 */
void keyboard_send_command(uint8_t command) {
    // Wait for input buffer to be empty
    while ((inb(KEYBOARD_COMMAND_PORT) & 0x02) != 0) {
        // Busy wait
    }
    
    outb(KEYBOARD_COMMAND_PORT, command);
}

/**
 * Send data to keyboard
 */
void keyboard_send_data(uint8_t data) {
    // Wait for input buffer to be empty
    while ((inb(KEYBOARD_COMMAND_PORT) & 0x02) != 0) {
        // Busy wait
    }
    
    outb(KEYBOARD_DATA_PORT, data);
}

/**
 * Read data from keyboard
 */
uint8_t keyboard_read_data(void) {
    // Wait for output buffer to be full
    while ((inb(KEYBOARD_COMMAND_PORT) & 0x01) == 0) {
        // Busy wait
    }
    
    return inb(KEYBOARD_DATA_PORT);
}

/**
 * Handle keyboard interrupt
 */
void keyboard_handle_interrupt(void) {
    if (!keyboard_initialized) {
        return;
    }
    
    uint8_t scancode = keyboard_read_data();
    
    // Check for extended key (0xE0 prefix)
    static bool extended_key = false;
    if (scancode == 0xE0) {
        extended_key = true;
        return;
    }
    
    // Check for key release (0x80 bit set)
    bool key_pressed = (scancode & 0x80) == 0;
    uint8_t key_code = scancode & 0x7F;
    
    // Create keyboard event
    keyboard_event_t event;
    event.key_code = key_code;
    event.pressed = key_pressed;
    event.is_special = false;
    event.ascii_char = 0;
    
    // Handle special keys
    switch (key_code) {
        case KEY_SHIFT_LEFT:
        case KEY_SHIFT_RIGHT:
            keyboard_state.shift_pressed = key_pressed;
            event.is_special = true;
            break;
            
        case KEY_CTRL:
            keyboard_state.ctrl_pressed = key_pressed;
            event.is_special = true;
            break;
            
        case KEY_ALT:
            keyboard_state.alt_pressed = key_pressed;
            event.is_special = true;
            break;
            
        case KEY_CAPS_LOCK:
            if (key_pressed) {
                keyboard_state.caps_lock = !keyboard_state.caps_lock;
            }
            event.is_special = true;
            break;
            
        case KEY_NUM_LOCK:
            if (key_pressed) {
                keyboard_state.num_lock = !keyboard_state.num_lock;
            }
            event.is_special = true;
            break;
            
        case KEY_SCROLL_LOCK:
            if (key_pressed) {
                keyboard_state.scroll_lock = !keyboard_state.scroll_lock;
            }
            event.is_special = true;
            break;
            
        default:
            // Handle regular keys
            if (key_pressed && key_code < 128) {
                bool shift_active = keyboard_state.shift_pressed ^ keyboard_state.caps_lock;
                event.ascii_char = shift_active ? ascii_table_shift[key_code] : ascii_table[key_code];
            }
            break;
    }
    
    // Add event to buffer
    keyboard_add_event(&event);
    
    // Reset extended key flag
    extended_key = false;
}

/**
 * Add event to keyboard buffer
 */
void keyboard_add_event(keyboard_event_t* event) {
    int next_head = (buffer_head + 1) % KEYBOARD_BUFFER_SIZE;
    
    if (next_head != buffer_tail) {
        event_buffer[buffer_head] = *event;
        buffer_head = next_head;
    }
}

/**
 * Get keyboard event
 */
bool keyboard_get_event(keyboard_event_t* event) {
    if (buffer_head == buffer_tail) {
        return false;  // No events available
    }
    
    *event = event_buffer[buffer_tail];
    buffer_tail = (buffer_tail + 1) % KEYBOARD_BUFFER_SIZE;
    
    return true;
}

/**
 * Check if key is pressed
 */
bool keyboard_is_key_pressed(uint8_t key_code) {
    // This is a simplified implementation
    // In a real implementation, you would maintain a key state table
    return false;
}

/**
 * Get keyboard state
 */
keyboard_state_t* keyboard_get_state(void) {
    return &keyboard_state;
}

/**
 * Read character (blocking)
 */
char keyboard_read_char(void) {
    keyboard_event_t event;
    
    while (1) {
        if (keyboard_get_event(&event)) {
            if (event.pressed && event.ascii_char != 0) {
                return event.ascii_char;
            }
        }
        
        // Small delay to prevent busy waiting
        for (volatile int i = 0; i < 1000; i++) {
            __asm__ volatile("nop");
        }
    }
}

/**
 * Read string (blocking)
 */
void keyboard_read_string(char* buffer, size_t max_length) {
    size_t length = 0;
    
    while (length < max_length - 1) {
        char c = keyboard_read_char();
        
        if (c == '\n' || c == '\r') {
            break;
        }
        
        if (c == '\b' && length > 0) {
            length--;
            // In a real implementation, you would also clear the character from display
            continue;
        }
        
        if (c >= 32 && c <= 126) {  // Printable ASCII
            buffer[length++] = c;
        }
    }
    
    buffer[length] = '\0';
}

/**
 * Check if keyboard buffer is empty
 */
bool keyboard_buffer_empty(void) {
    return buffer_head == buffer_tail;
}

/**
 * Clear keyboard buffer
 */
void keyboard_clear_buffer(void) {
    buffer_head = buffer_tail = 0;
    memset(event_buffer, 0, sizeof(event_buffer));
}

/**
 * Get buffer status
 */
void keyboard_get_buffer_status(int* head, int* tail, int* size) {
    if (head) *head = buffer_head;
    if (tail) *tail = buffer_tail;
    if (size) *size = KEYBOARD_BUFFER_SIZE;
}

/**
 * Input byte from port
 */
static uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

/**
 * Output byte to port
 */
static void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
} 