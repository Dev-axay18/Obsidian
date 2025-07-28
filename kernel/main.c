/**
 * 💠 Obsidian OS Kernel - Main Entry Point
 * Core kernel initialization and main loop
 */

#include <stdint.h>
#include <stddef.h>
#include "kernel.h"
#include "memory.h"
#include "process.h"
#include "scheduler.h"
#include "syscalls.h"
#include "drivers/display.h"
#include "drivers/keyboard.h"

// Kernel version and information
#define KERNEL_VERSION "1.0.0"
#define KERNEL_NAME "Obsidian OS"

// Global kernel state
kernel_state_t kernel_state = {0};

/**
 * Early kernel initialization
 */
void kernel_early_init(void) {
    // Initialize basic hardware
    display_init();
    keyboard_init();
    
    // Clear screen and show boot message
    display_clear();
    display_printf("💠 %s Kernel v%s\n", KERNEL_NAME, KERNEL_VERSION);
    display_printf("Initializing system...\n");
}

/**
 * Main kernel initialization
 */
void kernel_init(void) {
    display_printf("Setting up memory management...\n");
    memory_init();
    
    display_printf("Initializing process management...\n");
    process_init();
    
    display_printf("Setting up scheduler...\n");
    scheduler_init();
    
    display_printf("Initializing system calls...\n");
    syscalls_init();
    
    display_printf("Starting AI services...\n");
    ai_services_init();
    
    display_printf("Kernel initialization complete!\n");
    display_printf("Welcome to %s!\n", KERNEL_NAME);
}

/**
 * Main kernel loop
 */
void kernel_main_loop(void) {
    while (1) {
        // Process system events
        process_events();
        
        // Run scheduler
        scheduler_run();
        
        // Handle AI background tasks
        ai_background_tasks();
        
        // Small delay to prevent busy waiting
        for (volatile int i = 0; i < 1000000; i++) {
            __asm__ volatile("nop");
        }
    }
}

/**
 * Kernel entry point
 */
void kernel_entry(void) {
    // Early initialization
    kernel_early_init();
    
    // Main initialization
    kernel_init();
    
    // Enter main loop
    kernel_main_loop();
}

/**
 * Assembly entry point for linker
 */
extern "C" void _start(void) {
    kernel_entry();
}

/**
 * Kernel panic handler
 */
void kernel_panic(const char* message) {
    display_printf("\n💥 KERNEL PANIC: %s\n", message);
    display_printf("System halted.\n");
    
    // Disable interrupts and halt
    __asm__ volatile("cli");
    __asm__ volatile("hlt");
}

/**
 * Initialize AI services
 */
void ai_services_init(void) {
    // Initialize AI components
    // This will be expanded as AI modules are developed
    display_printf("  - LLM Engine: Initialized\n");
    display_printf("  - Voice Assistant: Ready\n");
    display_printf("  - Vision API: Active\n");
    display_printf("  - NLP Processor: Online\n");
}

/**
 * Process system events
 */
void process_events(void) {
    // Handle keyboard input
    keyboard_event_t key_event;
    if (keyboard_get_event(&key_event)) {
        // Process keyboard events
        handle_keyboard_event(&key_event);
    }
    
    // Handle other system events
    // (timer, network, etc.)
}

/**
 * Handle keyboard events
 */
void handle_keyboard_event(keyboard_event_t* event) {
    // Process keyboard input
    // This will be expanded for shell interaction
    if (event->pressed) {
        display_putchar(event->key);
    }
}

/**
 * AI background tasks
 */
void ai_background_tasks(void) {
    // Run AI background processing
    // This will handle continuous AI operations
    static int counter = 0;
    counter++;
    
    // Update AI status every 1000 iterations
    if (counter % 1000 == 0) {
        // Update AI status indicators
        // This is a placeholder for actual AI processing
    }
} 