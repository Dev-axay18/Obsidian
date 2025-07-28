/**
 * 💠 Obsidian OS Kernel - Process Management
 * Process creation, scheduling, and context switching
 */

#include "kernel.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

// Process control block structure
typedef struct {
    uint64_t rax, rbx, rcx, rdx;
    uint64_t rsi, rdi, rbp, rsp;
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
    uint64_t rip, rflags;
    uint64_t cr3;  // Page directory
} process_context_t;

// Process structure (extended from kernel.h)
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
    process_context_t context;
    uint64_t wake_time;
    uint32_t quantum;
    bool ai_priority;
} process_t;

// Global variables
static process_t processes[MAX_PROCESSES];
static uint32_t next_pid = 1;
static process_t* current_process = NULL;
static uint32_t total_processes = 0;
static uint32_t active_processes = 0;

// Process queues
static process_t* ready_queue[MAX_PROCESSES];
static int ready_queue_head = 0;
static int ready_queue_tail = 0;

static process_t* waiting_queue[MAX_PROCESSES];
static int waiting_queue_head = 0;
static int waiting_queue_tail = 0;

/**
 * Initialize process management system
 */
void process_init(void) {
    // Clear process table
    memset(processes, 0, sizeof(processes));
    
    // Initialize queues
    ready_queue_head = ready_queue_tail = 0;
    waiting_queue_head = waiting_queue_tail = 0;
    
    // Create idle process
    process_create("idle", NULL);
    
    total_processes = 0;
    active_processes = 0;
    current_process = NULL;
}

/**
 * Create a new process
 */
int process_create(const char* name, void* entry_point) {
    if (total_processes >= MAX_PROCESSES) {
        return -1;  // Too many processes
    }
    
    // Find free process slot
    int slot = -1;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i].state == PROCESS_TERMINATED || processes[i].state == 0) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        return -1;  // No free slots
    }
    
    process_t* process = &processes[slot];
    
    // Initialize process
    process->pid = next_pid++;
    process->state = PROCESS_NEW;
    process->priority = 5;  // Default priority
    process->parent_pid = current_process ? current_process->pid : 0;
    process->cpu_time = 0;
    process->memory_usage = 0;
    process->wake_time = 0;
    process->quantum = 10;  // Default time quantum
    process->ai_priority = false;
    
    // Copy name
    strncpy(process->name, name, sizeof(process->name) - 1);
    process->name[sizeof(process->name) - 1] = '\0';
    
    // Allocate stack
    process->stack_size = KERNEL_STACK_SIZE;
    process->stack = kmalloc(process->stack_size);
    
    if (!process->stack) {
        process->state = PROCESS_TERMINATED;
        return -1;  // Out of memory
    }
    
    // Initialize stack
    uint64_t* stack_top = (uint64_t*)((char*)process->stack + process->stack_size);
    
    // Set up initial stack frame
    *--stack_top = 0;  // Return address (will be set by scheduler)
    *--stack_top = 0;  // RBP
    *--stack_top = 0;  // R12
    *--stack_top = 0;  // R13
    *--stack_top = 0;  // R14
    *--stack_top = 0;  // R15
    
    // Initialize context
    process->context.rsp = (uint64_t)stack_top;
    process->context.rbp = (uint64_t)stack_top;
    process->context.rip = entry_point ? (uint64_t)entry_point : 0;
    process->context.rflags = 0x202;  // Interrupts enabled
    process->context.cr3 = 0;  // Will be set by memory management
    
    process->entry_point = entry_point;
    process->memory_usage = process->stack_size;
    
    total_processes++;
    active_processes++;
    
    // Add to ready queue
    process->state = PROCESS_READY;
    ready_queue[ready_queue_tail] = process;
    ready_queue_tail = (ready_queue_tail + 1) % MAX_PROCESSES;
    
    return process->pid;
}

/**
 * Destroy a process
 */
void process_destroy(uint32_t pid) {
    process_t* process = process_get_by_pid(pid);
    if (!process) {
        return;
    }
    
    // Free resources
    if (process->stack) {
        kfree(process->stack);
        process->stack = NULL;
    }
    
    // Remove from queues
    remove_from_ready_queue(process);
    remove_from_waiting_queue(process);
    
    // Mark as terminated
    process->state = PROCESS_TERMINATED;
    active_processes--;
    
    // If this was the current process, schedule another
    if (current_process == process) {
        current_process = NULL;
    }
}

/**
 * Get process by PID
 */
process_t* process_get_by_pid(uint32_t pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i].pid == pid && processes[i].state != PROCESS_TERMINATED) {
            return &processes[i];
        }
    }
    return NULL;
}

/**
 * Get current process
 */
process_t* process_get_current(void) {
    return current_process;
}

/**
 * Set process priority
 */
void process_set_priority(uint32_t pid, uint32_t priority) {
    process_t* process = process_get_by_pid(pid);
    if (process) {
        process->priority = priority;
    }
}

/**
 * Set AI priority flag
 */
void process_set_ai_priority(uint32_t pid, bool ai_priority) {
    process_t* process = process_get_by_pid(pid);
    if (process) {
        process->ai_priority = ai_priority;
    }
}

/**
 * Put process to sleep
 */
void process_sleep(uint32_t pid, uint64_t milliseconds) {
    process_t* process = process_get_by_pid(pid);
    if (!process) {
        return;
    }
    
    // Remove from ready queue
    remove_from_ready_queue(process);
    
    // Add to waiting queue
    process->state = PROCESS_WAITING;
    process->wake_time = get_uptime() + milliseconds;
    waiting_queue[waiting_queue_tail] = process;
    waiting_queue_tail = (waiting_queue_tail + 1) % MAX_PROCESSES;
}

/**
 * Wake up a process
 */
void process_wake(uint32_t pid) {
    process_t* process = process_get_by_pid(pid);
    if (!process || process->state != PROCESS_WAITING) {
        return;
    }
    
    // Remove from waiting queue
    remove_from_waiting_queue(process);
    
    // Add to ready queue
    process->state = PROCESS_READY;
    ready_queue[ready_queue_tail] = process;
    ready_queue_tail = (ready_queue_tail + 1) % MAX_PROCESSES;
}

/**
 * Get next process from ready queue
 */
process_t* process_get_next_ready(void) {
    if (ready_queue_head == ready_queue_tail) {
        return NULL;  // No ready processes
    }
    
    process_t* process = ready_queue[ready_queue_head];
    ready_queue_head = (ready_queue_head + 1) % MAX_PROCESSES;
    
    return process;
}

/**
 * Add process to ready queue
 */
void process_add_to_ready(process_t* process) {
    if (!process) {
        return;
    }
    
    process->state = PROCESS_READY;
    ready_queue[ready_queue_tail] = process;
    ready_queue_tail = (ready_queue_tail + 1) % MAX_PROCESSES;
}

/**
 * Remove process from ready queue
 */
void remove_from_ready_queue(process_t* process) {
    for (int i = ready_queue_head; i != ready_queue_tail; i = (i + 1) % MAX_PROCESSES) {
        if (ready_queue[i] == process) {
            // Shift remaining elements
            for (int j = i; j != ready_queue_tail; j = (j + 1) % MAX_PROCESSES) {
                ready_queue[j] = ready_queue[(j + 1) % MAX_PROCESSES];
            }
            ready_queue_tail = (ready_queue_tail - 1 + MAX_PROCESSES) % MAX_PROCESSES;
            break;
        }
    }
}

/**
 * Remove process from waiting queue
 */
void remove_from_waiting_queue(process_t* process) {
    for (int i = waiting_queue_head; i != waiting_queue_tail; i = (i + 1) % MAX_PROCESSES) {
        if (waiting_queue[i] == process) {
            // Shift remaining elements
            for (int j = i; j != waiting_queue_tail; j = (j + 1) % MAX_PROCESSES) {
                waiting_queue[j] = waiting_queue[(j + 1) % MAX_PROCESSES];
            }
            waiting_queue_tail = (waiting_queue_tail - 1 + MAX_PROCESSES) % MAX_PROCESSES;
            break;
        }
    }
}

/**
 * Check for processes to wake up
 */
void process_check_wakeups(void) {
    uint64_t current_time = get_uptime();
    
    for (int i = waiting_queue_head; i != waiting_queue_tail; i = (i + 1) % MAX_PROCESSES) {
        process_t* process = waiting_queue[i];
        if (process && process->wake_time <= current_time) {
            process_wake(process->pid);
        }
    }
}

/**
 * Get process statistics
 */
void process_get_stats(uint32_t* total, uint32_t* active, uint32_t* ready, uint32_t* waiting) {
    if (total) *total = total_processes;
    if (active) *active = active_processes;
    if (ready) *ready = (ready_queue_tail - ready_queue_head + MAX_PROCESSES) % MAX_PROCESSES;
    if (waiting) *waiting = (waiting_queue_tail - waiting_queue_head + MAX_PROCESSES) % MAX_PROCESSES;
}

/**
 * List all processes
 */
void process_list_all(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i].state != PROCESS_TERMINATED && processes[i].state != 0) {
            process_t* p = &processes[i];
            // This would print process info - for now just update display
            // In a real implementation, this would use a display driver
        }
    }
}

/**
 * Save current process context
 */
void process_save_context(process_context_t* context) {
    if (!context) {
        return;
    }
    
    // Save general purpose registers
    __asm__ volatile(
        "mov %%rax, %0\n"
        "mov %%rbx, %1\n"
        "mov %%rcx, %2\n"
        "mov %%rdx, %3\n"
        "mov %%rsi, %4\n"
        "mov %%rdi, %5\n"
        "mov %%rbp, %6\n"
        "mov %%rsp, %7\n"
        "mov %%r8, %8\n"
        "mov %%r9, %9\n"
        "mov %%r10, %10\n"
        "mov %%r11, %11\n"
        "mov %%r12, %12\n"
        "mov %%r13, %13\n"
        "mov %%r14, %14\n"
        "mov %%r15, %15\n"
        : "=m"(context->rax), "=m"(context->rbx), "=m"(context->rcx), "=m"(context->rdx),
          "=m"(context->rsi), "=m"(context->rdi), "=m"(context->rbp), "=m"(context->rsp),
          "=m"(context->r8), "=m"(context->r9), "=m"(context->r10), "=m"(context->r11),
          "=m"(context->r12), "=m"(context->r13), "=m"(context->r14), "=m"(context->r15)
        :
        : "memory"
    );
    
    // Save RIP and RFLAGS
    __asm__ volatile(
        "lea 1f, %%rax\n"
        "mov %%rax, %0\n"
        "pushfq\n"
        "pop %%rax\n"
        "mov %%rax, %1\n"
        "1:\n"
        : "=m"(context->rip), "=m"(context->rflags)
        :
        : "rax", "memory"
    );
    
    // Save CR3
    __asm__ volatile("mov %%cr3, %0" : "=r"(context->cr3));
}

/**
 * Restore process context
 */
void process_restore_context(process_context_t* context) {
    if (!context) {
        return;
    }
    
    // Restore CR3
    __asm__ volatile("mov %0, %%cr3" : : "r"(context->cr3));
    
    // Restore general purpose registers
    __asm__ volatile(
        "mov %0, %%rax\n"
        "mov %1, %%rbx\n"
        "mov %2, %%rcx\n"
        "mov %3, %%rdx\n"
        "mov %4, %%rsi\n"
        "mov %5, %%rdi\n"
        "mov %6, %%rbp\n"
        "mov %7, %%rsp\n"
        "mov %8, %%r8\n"
        "mov %9, %%r9\n"
        "mov %10, %%r10\n"
        "mov %11, %%r11\n"
        "mov %12, %%r12\n"
        "mov %13, %%r13\n"
        "mov %14, %%r14\n"
        "mov %15, %%r15\n"
        : : "m"(context->rax), "m"(context->rbx), "m"(context->rcx), "m"(context->rdx),
            "m"(context->rsi), "m"(context->rdi), "m"(context->rbp), "m"(context->rsp),
            "m"(context->r8), "m"(context->r9), "m"(context->r10), "m"(context->r11),
            "m"(context->r12), "m"(context->r13), "m"(context->r14), "m"(context->r15)
        : "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp", "rsp",
          "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"
    );
    
    // Restore RFLAGS and jump to RIP
    __asm__ volatile(
        "push %0\n"
        "popfq\n"
        "jmp *%1\n"
        : : "r"(context->rflags), "r"(context->rip)
        : "memory"
    );
} 