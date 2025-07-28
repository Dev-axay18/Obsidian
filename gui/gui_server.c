/**
 * 💠 Obsidian OS - GUI Server
 * Basic GUI server with window management
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

// GUI constants
#define GUI_MAX_WINDOWS 32
#define GUI_MAX_WIDGETS 256
#define GUI_SCREEN_WIDTH 1024
#define GUI_SCREEN_HEIGHT 768
#define GUI_COLOR_DEPTH 32

// Color definitions
#define GUI_COLOR_BLACK 0x000000
#define GUI_COLOR_WHITE 0xFFFFFF
#define GUI_COLOR_RED 0xFF0000
#define GUI_COLOR_GREEN 0x00FF00
#define GUI_COLOR_BLUE 0x0000FF
#define GUI_COLOR_GRAY 0x808080
#define GUI_COLOR_LIGHT_GRAY 0xC0C0C0
#define GUI_COLOR_DARK_GRAY 0x404040

// Widget types
typedef enum {
    WIDGET_BUTTON,
    WIDGET_LABEL,
    WIDGET_TEXTBOX,
    WIDGET_LISTBOX,
    WIDGET_CHECKBOX,
    WIDGET_RADIOBUTTON,
    WIDGET_PROGRESSBAR,
    WIDGET_SCROLLBAR
} widget_type_t;

// Widget states
typedef enum {
    WIDGET_STATE_NORMAL,
    WIDGET_STATE_HOVER,
    WIDGET_STATE_PRESSED,
    WIDGET_STATE_DISABLED
} widget_state_t;

// Rectangle structure
typedef struct {
    int x, y;
    int width, height;
} gui_rect_t;

// Widget structure
typedef struct {
    uint32_t id;
    widget_type_t type;
    widget_state_t state;
    gui_rect_t bounds;
    char text[256];
    bool visible;
    bool enabled;
    uint32_t color;
    uint32_t bg_color;
    void* data;
} gui_widget_t;

// Window structure
typedef struct {
    uint32_t id;
    char title[256];
    gui_rect_t bounds;
    bool visible;
    bool focused;
    bool resizable;
    bool movable;
    uint32_t bg_color;
    gui_widget_t* widgets[64];
    int widget_count;
} gui_window_t;

// GUI server state
typedef struct {
    gui_window_t* windows[GUI_MAX_WINDOWS];
    int window_count;
    uint32_t* framebuffer;
    uint32_t focused_window;
    bool mouse_pressed;
    int mouse_x, mouse_y;
    int last_mouse_x, last_mouse_y;
} gui_server_t;

// Global variables
static gui_server_t gui_server = {0};
static uint32_t next_window_id = 1;
static uint32_t next_widget_id = 1;

/**
 * Initialize GUI server
 */
int gui_server_init(void) {
    // Clear server state
    memset(&gui_server, 0, sizeof(gui_server));
    
    // Allocate framebuffer
    gui_server.framebuffer = (uint32_t*)malloc(GUI_SCREEN_WIDTH * GUI_SCREEN_HEIGHT * 4);
    if (!gui_server.framebuffer) {
        return -1;
    }
    
    // Clear framebuffer
    memset(gui_server.framebuffer, 0, GUI_SCREEN_WIDTH * GUI_SCREEN_HEIGHT * 4);
    
    printf("✅ GUI Server initialized\n");
    return 0;
}

/**
 * Shutdown GUI server
 */
void gui_server_shutdown(void) {
    // Close all windows
    for (int i = 0; i < gui_server.window_count; i++) {
        if (gui_server.windows[i]) {
            gui_window_destroy(gui_server.windows[i]->id);
        }
    }
    
    // Free framebuffer
    if (gui_server.framebuffer) {
        free(gui_server.framebuffer);
        gui_server.framebuffer = NULL;
    }
    
    printf("✅ GUI Server shutdown\n");
}

/**
 * Create window
 */
uint32_t gui_window_create(const char* title, int x, int y, int width, int height) {
    if (gui_server.window_count >= GUI_MAX_WINDOWS) {
        return 0;  // Too many windows
    }
    
    gui_window_t* window = (gui_window_t*)malloc(sizeof(gui_window_t));
    if (!window) {
        return 0;
    }
    
    // Initialize window
    window->id = next_window_id++;
    strncpy(window->title, title, sizeof(window->title) - 1);
    window->bounds.x = x;
    window->bounds.y = y;
    window->bounds.width = width;
    window->bounds.height = height;
    window->visible = true;
    window->focused = false;
    window->resizable = true;
    window->movable = true;
    window->bg_color = GUI_COLOR_LIGHT_GRAY;
    window->widget_count = 0;
    
    // Add to server
    gui_server.windows[gui_server.window_count++] = window;
    
    printf("✅ Window created: %s (ID: %u)\n", title, window->id);
    return window->id;
}

/**
 * Destroy window
 */
void gui_window_destroy(uint32_t window_id) {
    for (int i = 0; i < gui_server.window_count; i++) {
        if (gui_server.windows[i] && gui_server.windows[i]->id == window_id) {
            gui_window_t* window = gui_server.windows[i];
            
            // Free widgets
            for (int j = 0; j < window->widget_count; j++) {
                if (window->widgets[j]) {
                    free(window->widgets[j]);
                }
            }
            
            // Remove from server
            free(window);
            gui_server.windows[i] = NULL;
            
            // Shift remaining windows
            for (int j = i; j < gui_server.window_count - 1; j++) {
                gui_server.windows[j] = gui_server.windows[j + 1];
            }
            gui_server.window_count--;
            
            printf("✅ Window destroyed: ID %u\n", window_id);
            return;
        }
    }
}

/**
 * Get window by ID
 */
gui_window_t* gui_window_get(uint32_t window_id) {
    for (int i = 0; i < gui_server.window_count; i++) {
        if (gui_server.windows[i] && gui_server.windows[i]->id == window_id) {
            return gui_server.windows[i];
        }
    }
    return NULL;
}

/**
 * Set window visibility
 */
void gui_window_set_visible(uint32_t window_id, bool visible) {
    gui_window_t* window = gui_window_get(window_id);
    if (window) {
        window->visible = visible;
    }
}

/**
 * Set window focus
 */
void gui_window_set_focus(uint32_t window_id) {
    // Remove focus from current window
    if (gui_server.focused_window) {
        gui_window_t* current = gui_window_get(gui_server.focused_window);
        if (current) {
            current->focused = false;
        }
    }
    
    // Set focus to new window
    gui_window_t* window = gui_window_get(window_id);
    if (window) {
        window->focused = true;
        gui_server.focused_window = window_id;
    }
}

/**
 * Create widget
 */
uint32_t gui_widget_create(uint32_t window_id, widget_type_t type, const char* text, 
                          int x, int y, int width, int height) {
    gui_window_t* window = gui_window_get(window_id);
    if (!window || window->widget_count >= 64) {
        return 0;
    }
    
    gui_widget_t* widget = (gui_widget_t*)malloc(sizeof(gui_widget_t));
    if (!widget) {
        return 0;
    }
    
    // Initialize widget
    widget->id = next_widget_id++;
    widget->type = type;
    widget->state = WIDGET_STATE_NORMAL;
    widget->bounds.x = x;
    widget->bounds.y = y;
    widget->bounds.width = width;
    widget->bounds.height = height;
    strncpy(widget->text, text, sizeof(widget->text) - 1);
    widget->visible = true;
    widget->enabled = true;
    widget->color = GUI_COLOR_BLACK;
    widget->bg_color = GUI_COLOR_WHITE;
    widget->data = NULL;
    
    // Add to window
    window->widgets[window->widget_count++] = widget;
    
    printf("✅ Widget created: %s (ID: %u) in window %u\n", text, widget->id, window_id);
    return widget->id;
}

/**
 * Get widget by ID
 */
gui_widget_t* gui_widget_get(uint32_t window_id, uint32_t widget_id) {
    gui_window_t* window = gui_window_get(window_id);
    if (!window) {
        return NULL;
    }
    
    for (int i = 0; i < window->widget_count; i++) {
        if (window->widgets[i] && window->widgets[i]->id == widget_id) {
            return window->widgets[i];
        }
    }
    
    return NULL;
}

/**
 * Set widget text
 */
void gui_widget_set_text(uint32_t window_id, uint32_t widget_id, const char* text) {
    gui_widget_t* widget = gui_widget_get(window_id, widget_id);
    if (widget) {
        strncpy(widget->text, text, sizeof(widget->text) - 1);
    }
}

/**
 * Set widget state
 */
void gui_widget_set_state(uint32_t window_id, uint32_t widget_id, widget_state_t state) {
    gui_widget_t* widget = gui_widget_get(window_id, widget_id);
    if (widget) {
        widget->state = state;
    }
}

/**
 * Draw rectangle
 */
void gui_draw_rect(int x, int y, int width, int height, uint32_t color) {
    if (x < 0 || y < 0 || x + width > GUI_SCREEN_WIDTH || y + height > GUI_SCREEN_HEIGHT) {
        return;
    }
    
    for (int dy = 0; dy < height; dy++) {
        for (int dx = 0; dx < width; dx++) {
            gui_server.framebuffer[(y + dy) * GUI_SCREEN_WIDTH + (x + dx)] = color;
        }
    }
}

/**
 * Draw text (simple implementation)
 */
void gui_draw_text(int x, int y, const char* text, uint32_t color) {
    // Simple text rendering - just draw colored rectangles for each character
    int char_width = 8;
    int char_height = 16;
    
    for (int i = 0; text[i] != '\0'; i++) {
        int char_x = x + i * char_width;
        int char_y = y;
        
        // Draw character as a simple rectangle
        gui_draw_rect(char_x, char_y, char_width, char_height, color);
    }
}

/**
 * Draw window
 */
void gui_draw_window(gui_window_t* window) {
    if (!window || !window->visible) {
        return;
    }
    
    // Draw window background
    gui_draw_rect(window->bounds.x, window->bounds.y, 
                 window->bounds.width, window->bounds.height, 
                 window->bg_color);
    
    // Draw window border
    uint32_t border_color = window->focused ? GUI_COLOR_BLUE : GUI_COLOR_GRAY;
    gui_draw_rect(window->bounds.x, window->bounds.y, window->bounds.width, 2, border_color);
    gui_draw_rect(window->bounds.x, window->bounds.y, 2, window->bounds.height, border_color);
    gui_draw_rect(window->bounds.x + window->bounds.width - 2, window->bounds.y, 
                 2, window->bounds.height, border_color);
    gui_draw_rect(window->bounds.x, window->bounds.y + window->bounds.height - 2, 
                 window->bounds.width, 2, border_color);
    
    // Draw title bar
    gui_draw_rect(window->bounds.x, window->bounds.y, window->bounds.width, 20, GUI_COLOR_DARK_GRAY);
    gui_draw_text(window->bounds.x + 5, window->bounds.y + 2, window->title, GUI_COLOR_WHITE);
    
    // Draw widgets
    for (int i = 0; i < window->widget_count; i++) {
        gui_widget_t* widget = window->widgets[i];
        if (widget && widget->visible) {
            gui_draw_widget(widget, window->bounds.x, window->bounds.y);
        }
    }
}

/**
 * Draw widget
 */
void gui_draw_widget(gui_widget_t* widget, int window_x, int window_y) {
    if (!widget) {
        return;
    }
    
    int x = window_x + widget->bounds.x;
    int y = window_y + widget->bounds.y;
    
    switch (widget->type) {
        case WIDGET_BUTTON:
            // Draw button background
            uint32_t bg_color = widget->state == WIDGET_STATE_PRESSED ? 
                               GUI_COLOR_DARK_GRAY : GUI_COLOR_LIGHT_GRAY;
            gui_draw_rect(x, y, widget->bounds.width, widget->bounds.height, bg_color);
            
            // Draw button border
            gui_draw_rect(x, y, widget->bounds.width, 1, GUI_COLOR_BLACK);
            gui_draw_rect(x, y, 1, widget->bounds.height, GUI_COLOR_BLACK);
            gui_draw_rect(x + widget->bounds.width - 1, y, 1, widget->bounds.height, GUI_COLOR_BLACK);
            gui_draw_rect(x, y + widget->bounds.height - 1, widget->bounds.width, 1, GUI_COLOR_BLACK);
            
            // Draw button text
            gui_draw_text(x + 5, y + 5, widget->text, widget->color);
            break;
            
        case WIDGET_LABEL:
            gui_draw_text(x, y, widget->text, widget->color);
            break;
            
        case WIDGET_TEXTBOX:
            // Draw textbox background
            gui_draw_rect(x, y, widget->bounds.width, widget->bounds.height, GUI_COLOR_WHITE);
            
            // Draw textbox border
            gui_draw_rect(x, y, widget->bounds.width, 1, GUI_COLOR_BLACK);
            gui_draw_rect(x, y, 1, widget->bounds.height, GUI_COLOR_BLACK);
            gui_draw_rect(x + widget->bounds.width - 1, y, 1, widget->bounds.height, GUI_COLOR_BLACK);
            gui_draw_rect(x, y + widget->bounds.height - 1, widget->bounds.width, 1, GUI_COLOR_BLACK);
            
            // Draw textbox text
            gui_draw_text(x + 2, y + 2, widget->text, widget->color);
            break;
            
        default:
            // Draw generic widget
            gui_draw_rect(x, y, widget->bounds.width, widget->bounds.height, widget->bg_color);
            gui_draw_text(x + 2, y + 2, widget->text, widget->color);
            break;
    }
}

/**
 * Handle mouse event
 */
void gui_handle_mouse(int x, int y, bool pressed) {
    gui_server.mouse_x = x;
    gui_server.mouse_y = y;
    
    if (pressed && !gui_server.mouse_pressed) {
        // Mouse button pressed
        gui_server.mouse_pressed = true;
        
        // Check for window focus
        for (int i = gui_server.window_count - 1; i >= 0; i--) {
            gui_window_t* window = gui_server.windows[i];
            if (window && window->visible) {
                if (x >= window->bounds.x && x < window->bounds.x + window->bounds.width &&
                    y >= window->bounds.y && y < window->bounds.y + window->bounds.height) {
                    gui_window_set_focus(window->id);
                    break;
                }
            }
        }
        
        // Check for widget interaction
        gui_window_t* focused_window = gui_window_get(gui_server.focused_window);
        if (focused_window) {
            for (int i = 0; i < focused_window->widget_count; i++) {
                gui_widget_t* widget = focused_window->widgets[i];
                if (widget && widget->visible && widget->enabled) {
                    int widget_x = focused_window->bounds.x + widget->bounds.x;
                    int widget_y = focused_window->bounds.y + widget->bounds.y;
                    
                    if (x >= widget_x && x < widget_x + widget->bounds.width &&
                        y >= widget_y && y < widget_y + widget->bounds.height) {
                        widget->state = WIDGET_STATE_PRESSED;
                        break;
                    }
                }
            }
        }
    } else if (!pressed && gui_server.mouse_pressed) {
        // Mouse button released
        gui_server.mouse_pressed = false;
        
        // Reset widget states
        for (int i = 0; i < gui_server.window_count; i++) {
            gui_window_t* window = gui_server.windows[i];
            if (window) {
                for (int j = 0; j < window->widget_count; j++) {
                    gui_widget_t* widget = window->widgets[j];
                    if (widget && widget->state == WIDGET_STATE_PRESSED) {
                        widget->state = WIDGET_STATE_NORMAL;
                    }
                }
            }
        }
    }
    
    gui_server.last_mouse_x = gui_server.mouse_x;
    gui_server.last_mouse_y = gui_server.mouse_y;
}

/**
 * Render GUI
 */
void gui_render(void) {
    // Clear framebuffer
    memset(gui_server.framebuffer, 0, GUI_SCREEN_WIDTH * GUI_SCREEN_HEIGHT * 4);
    
    // Draw desktop background
    gui_draw_rect(0, 0, GUI_SCREEN_WIDTH, GUI_SCREEN_HEIGHT, GUI_COLOR_BLUE);
    
    // Draw all windows
    for (int i = 0; i < gui_server.window_count; i++) {
        if (gui_server.windows[i]) {
            gui_draw_window(gui_server.windows[i]);
        }
    }
    
    // In a real implementation, this would copy the framebuffer to video memory
    printf("🎨 GUI rendered\n");
}

/**
 * Get framebuffer
 */
uint32_t* gui_get_framebuffer(void) {
    return gui_server.framebuffer;
}

/**
 * Memory allocation function (placeholder)
 */
static void* malloc(size_t size) {
    // This would use the kernel's memory allocator
    return (void*)0x200000;  // Dummy address
}

/**
 * Memory free function (placeholder)
 */
static void free(void* ptr) {
    // This would use the kernel's memory allocator
    (void)ptr;  // Suppress unused parameter warning
} 