/**
 * 💠 Obsidian OS Kernel - Scheduler
 * AI-aware priority-based scheduler with context switching
 */

#include "kernel.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Scheduler configuration
#define SCHEDULER_TICK_MS 10
#define MAX_PRIORITY 10
#define MIN_PRIORITY 1
#define AI_PRIORITY_BOOST 2

// Scheduler state
typedef enum {
    SCHEDULER_IDLE,
    SCHEDULER_RUNNING,
    SCHEDULER_SLEEPING
} scheduler_state_t;

// Scheduler statistics
typedef struct {
    uint64_t total_switches;
    uint64_t ai_task_switches;
    uint64_t idle_time;
    uint64_t last_switch_time;
    uint32_t current_quantum;
} scheduler_stats_t;

// Global variables
static scheduler_state_t scheduler_state = SCHEDULER_IDLE;
static scheduler_stats_t scheduler_stats = {0};
static uint64_t scheduler_tick_count = 0;
static process_t* current_process = NULL;
static process_t* next_process = NULL;
static bool context_switch_pending = false;

// Priority queues for different priority levels
static process_t* priority_queues[MAX_PRIORITY + 1][MAX_PROCESSES];
static int priority_queue_tails[MAX_PRIORITY + 1] = {0};
static int priority_queue_heads[MAX_PRIORITY + 1] = {0};

/**
 * Initialize the scheduler
 */
void scheduler_init(void) {
    // Clear priority queues
    for (int i = 0; i <= MAX_PRIORITY; i++) {
        priority_queue_heads[i] = priority_queue_tails[i] = 0;
    }
    
    // Initialize scheduler state
    scheduler_state = SCHEDULER_IDLE;
    scheduler_stats.total_switches = 0;
    scheduler_stats.ai_task_switches = 0;
    scheduler_stats.idle_time = 0;
    scheduler_stats.last_switch_time = 0;
    scheduler_stats.current_quantum = 0;
    
    current_process = NULL;
    next_process = NULL;
    context_switch_pending = false;
    
    // Set up timer interrupt for scheduling
    // This would configure the PIT or APIC timer
    // For now, we'll use a simple counter
}

/**
 * Main scheduler run function
 */
void scheduler_run(void) {
    // Check for processes to wake up
    process_check_wakeups();
    
    // Update scheduler tick
    scheduler_tick_count++;
    
    // Check if we need to switch processes
    if (should_switch_process()) {
        schedule_next_process();
    }
    
    // Update statistics
    update_scheduler_stats();
}

/**
 * Determine if we should switch processes
 */
bool should_switch_process(void) {
    // If no current process, we should switch
    if (!current_process) {
        return true;
    }
    
    // If current process is no longer ready, switch
    if (current_process->state != PROCESS_RUNNING) {
        return true;
    }
    
    // If quantum expired, switch
    if (scheduler_stats.current_quantum >= current_process->quantum) {
        return true;
    }
    
    // If there's a higher priority process ready, switch
    if (has_higher_priority_process()) {
        return true;
    }
    
    return false;
}

/**
 * Check if there's a higher priority process ready
 */
bool has_higher_priority_process(void) {
    if (!current_process) {
        return false;
    }
    
    uint32_t current_priority = get_effective_priority(current_process);
    
    // Check higher priority queues
    for (int priority = MAX_PRIORITY; priority > current_priority; priority--) {
        if (priority_queue_heads[priority] != priority_queue_tails[priority]) {
            return true;
        }
    }
    
    return false;
}

/**
 * Get effective priority (including AI boost)
 */
uint32_t get_effective_priority(process_t* process) {
    if (!process) {
        return MIN_PRIORITY;
    }
    
    uint32_t priority = process->priority;
    
    // Apply AI priority boost
    if (process->ai_priority) {
        priority += AI_PRIORITY_BOOST;
        if (priority > MAX_PRIORITY) {
            priority = MAX_PRIORITY;
        }
    }
    
    return priority;
}

/**
 * Schedule the next process to run
 */
void schedule_next_process(void) {
    process_t* new_process = select_next_process();
    
    if (new_process != current_process) {
        // Perform context switch
        context_switch(current_process, new_process);
    }
    
    // Update current process
    current_process = new_process;
    if (current_process) {
        current_process->state = PROCESS_RUNNING;
        scheduler_stats.current_quantum = 0;
    }
}

/**
 * Select the next process to run
 */
process_t* select_next_process(void) {
    // Use AI-aware priority scheduling
    for (int priority = MAX_PRIORITY; priority >= MIN_PRIORITY; priority--) {
        if (priority_queue_heads[priority] != priority_queue_tails[priority]) {
            process_t* process = priority_queue_heads[priority];
            priority_queue_heads[priority] = (priority_queue_heads[priority] + 1) % MAX_PROCESSES;
            return process;
        }
    }
    
    // No ready processes, return idle process
    return process_get_by_pid(1);  // Assuming PID 1 is idle process
}

/**
 * Perform context switch between processes
 */
void context_switch(process_t* old_process, process_t* new_process) {
    if (old_process == new_process) {
        return;
    }
    
    // Save old process context
    if (old_process && old_process->state == PROCESS_RUNNING) {
        process_save_context(&old_process->context);
        old_process->state = PROCESS_READY;
        add_to_priority_queue(old_process);
    }
    
    // Restore new process context
    if (new_process) {
        process_restore_context(&new_process->context);
        new_process->state = PROCESS_RUNNING;
    }
    
    // Update statistics
    scheduler_stats.total_switches++;
    if (new_process && new_process->ai_priority) {
        scheduler_stats.ai_task_switches++;
    }
    scheduler_stats.last_switch_time = get_uptime();
}

/**
 * Add process to priority queue
 */
void add_to_priority_queue(process_t* process) {
    if (!process) {
        return;
    }
    
    uint32_t priority = get_effective_priority(process);
    
    if (priority_queue_tails[priority] < MAX_PROCESSES) {
        priority_queues[priority][priority_queue_tails[priority]] = process;
        priority_queue_tails[priority]++;
    }
}

/**
 * Remove process from priority queue
 */
void remove_from_priority_queue(process_t* process) {
    if (!process) {
        return;
    }
    
    uint32_t priority = get_effective_priority(process);
    
    for (int i = priority_queue_heads[priority]; i < priority_queue_tails[priority]; i++) {
        if (priority_queues[priority][i] == process) {
            // Shift remaining elements
            for (int j = i; j < priority_queue_tails[priority] - 1; j++) {
                priority_queues[priority][j] = priority_queues[priority][j + 1];
            }
            priority_queue_tails[priority]--;
            break;
        }
    }
}

/**
 * Add process to scheduler
 */
void scheduler_add_process(process_t* process) {
    if (!process) {
        return;
    }
    
    process->state = PROCESS_READY;
    add_to_priority_queue(process);
    
    // If this is the first process, start it immediately
    if (!current_process) {
        schedule_next_process();
    }
}

/**
 * Remove process from scheduler
 */
void scheduler_remove_process(uint32_t pid) {
    process_t* process = process_get_by_pid(pid);
    if (!process) {
        return;
    }
    
    remove_from_priority_queue(process);
    
    // If this was the current process, schedule another
    if (current_process == process) {
        current_process = NULL;
        schedule_next_process();
    }
}

/**
 * Set process priority
 */
void scheduler_set_priority(uint32_t pid, uint32_t priority) {
    process_t* process = process_get_by_pid(pid);
    if (!process) {
        return;
    }
    
    // Remove from current priority queue
    remove_from_priority_queue(process);
    
    // Update priority
    process->priority = priority;
    if (priority > MAX_PRIORITY) {
        process->priority = MAX_PRIORITY;
    } else if (priority < MIN_PRIORITY) {
        process->priority = MIN_PRIORITY;
    }
    
    // Add to new priority queue
    add_to_priority_queue(process);
}

/**
 * Set AI priority for process
 */
void scheduler_set_ai_priority(uint32_t pid, bool ai_priority) {
    process_t* process = process_get_by_pid(pid);
    if (!process) {
        return;
    }
    
    // Remove from current priority queue
    remove_from_priority_queue(process);
    
    // Update AI priority
    process->ai_priority = ai_priority;
    
    // Add to new priority queue
    add_to_priority_queue(process);
}

/**
 * Update scheduler statistics
 */
void update_scheduler_stats(void) {
    // Update quantum counter
    if (current_process && current_process->state == PROCESS_RUNNING) {
        scheduler_stats.current_quantum++;
    } else {
        scheduler_stats.idle_time++;
    }
}

/**
 * Get scheduler statistics
 */
void scheduler_get_stats(scheduler_stats_t* stats) {
    if (stats) {
        *stats = scheduler_stats;
    }
}

/**
 * Get current process
 */
process_t* scheduler_get_current_process(void) {
    return current_process;
}

/**
 * Get next process (without switching)
 */
process_t* scheduler_get_next_process(void) {
    return select_next_process();
}

/**
 * Yield current process
 */
void scheduler_yield(void) {
    if (current_process) {
        current_process->state = PROCESS_READY;
        add_to_priority_queue(current_process);
        schedule_next_process();
    }
}

/**
 * Sleep current process
 */
void scheduler_sleep(uint64_t milliseconds) {
    if (current_process) {
        process_sleep(current_process->pid, milliseconds);
        schedule_next_process();
    }
}

/**
 * Wake up a process
 */
void scheduler_wake(uint32_t pid) {
    process_t* process = process_get_by_pid(pid);
    if (process && process->state == PROCESS_WAITING) {
        process_wake(pid);
        add_to_priority_queue(process);
        
        // If this process has higher priority, switch to it
        if (current_process && get_effective_priority(process) > get_effective_priority(current_process)) {
            schedule_next_process();
        }
    }
}

/**
 * Get scheduler tick count
 */
uint64_t scheduler_get_tick_count(void) {
    return scheduler_tick_count;
}

/**
 * Check if scheduler is idle
 */
bool scheduler_is_idle(void) {
    return scheduler_state == SCHEDULER_IDLE;
}

/**
 * Get ready process count
 */
uint32_t scheduler_get_ready_count(void) {
    uint32_t count = 0;
    for (int i = MIN_PRIORITY; i <= MAX_PRIORITY; i++) {
        count += priority_queue_tails[i] - priority_queue_heads[i];
    }
    return count;
}

/**
 * List all processes in scheduler
 */
void scheduler_list_processes(void) {
    for (int priority = MAX_PRIORITY; priority >= MIN_PRIORITY; priority--) {
        for (int i = priority_queue_heads[priority]; i < priority_queue_tails[priority]; i++) {
            process_t* process = priority_queues[priority][i];
            if (process) {
                // This would print process info
                // In a real implementation, this would use a display driver
            }
        }
    }
} 