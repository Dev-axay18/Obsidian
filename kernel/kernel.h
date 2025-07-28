/**
 * 💠 Obsidian OS Kernel - Main Header
 * Core kernel definitions, structures, and function declarations
 */

#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Kernel configuration
#define KERNEL_STACK_SIZE 4096
#define MAX_PROCESSES 256
#define MAX_DEVICES 64
#define MAX_OPEN_FILES 1024

// Memory management
#define PAGE_SIZE 4096
#define KERNEL_HEAP_SIZE (1024 * 1024)  // 1MB
#define USER_HEAP_SIZE (16 * 1024 * 1024)  // 16MB

// System call numbers
#define SYS_READ 0
#define SYS_WRITE 1
#define SYS_OPEN 2
#define SYS_CLOSE 3
#define SYS_EXEC 4
#define SYS_EXIT 5
#define SYS_FORK 6
#define SYS_GETPID 7
#define SYS_SLEEP 8
#define SYS_AI_REQUEST 9

// Process states
typedef enum {
    PROCESS_NEW,
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_WAITING,
    PROCESS_TERMINATED
} process_state_t;

// Process structure
typedef struct {
    uint32_t pid;
    process_state_t state;
    uint32_t priority;
    void* stack;
    size_t stack_size;
    void* entry_point;
    char name[32];
    uint32_t parent_pid;
    uint64_t cpu_time;
    uint64_t memory_usage;
} process_t;

// Kernel state structure
typedef struct {
    uint32_t uptime;
    uint32_t total_processes;
    uint32_t active_processes;
    uint64_t total_memory;
    uint64_t free_memory;
    bool ai_services_ready;
    bool gui_ready;
} kernel_state_t;

// System call structure
typedef struct {
    uint32_t number;
    uint32_t args[6];
    uint32_t return_value;
} syscall_t;

// Device structure
typedef struct {
    uint32_t id;
    char name[32];
    uint32_t type;
    void* driver;
    bool active;
} device_t;

// AI request structure
typedef struct {
    uint32_t type;
    char* input;
    size_t input_size;
    char* output;
    size_t output_size;
    uint32_t priority;
} ai_request_t;

// Function declarations

// Kernel core functions
void kernel_entry(void);
void kernel_early_init(void);
void kernel_init(void);
void kernel_main_loop(void);
void kernel_panic(const char* message);

// AI services
void ai_services_init(void);
void ai_background_tasks(void);
int ai_process_request(ai_request_t* request);

// Event processing
void process_events(void);
void handle_keyboard_event(void* event);

// Memory management
void* kmalloc(size_t size);
void kfree(void* ptr);
void memory_init(void);

// Process management
int process_create(const char* name, void* entry_point);
void process_destroy(uint32_t pid);
void process_init(void);
process_t* process_get_current(void);

// Scheduler
void scheduler_init(void);
void scheduler_run(void);
void scheduler_add_process(process_t* process);
void scheduler_remove_process(uint32_t pid);

// System calls
void syscalls_init(void);
uint32_t syscall_handler(syscall_t* syscall);

// Device management
int device_register(device_t* device);
int device_unregister(uint32_t device_id);
device_t* device_get(uint32_t device_id);

// Utility functions
void delay(uint32_t milliseconds);
uint32_t get_uptime(void);
void reboot(void);
void shutdown(void);

// Debug functions
void debug_print(const char* message);
void debug_hexdump(const void* data, size_t size);

#endif // KERNEL_H 