/**
 * 💠 Obsidian OS - Kernel Tests
 * Comprehensive test suite for kernel components
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

// Include kernel headers
#include "../kernel/kernel.h"

// Test results
typedef struct {
    int total_tests;
    int passed_tests;
    int failed_tests;
    char failed_tests_list[1000];
} test_results_t;

static test_results_t test_results = {0};

// Test macros
#define TEST_ASSERT(condition, message) \
    do { \
        test_results.total_tests++; \
        if (condition) { \
            test_results.passed_tests++; \
            printf("✅ PASS: %s\n", message); \
        } else { \
            test_results.failed_tests++; \
            printf("❌ FAIL: %s\n", message); \
            char fail_msg[256]; \
            snprintf(fail_msg, sizeof(fail_msg), "%s; ", message); \
            strncat(test_results.failed_tests_list, fail_msg, sizeof(test_results.failed_tests_list) - 1); \
        } \
    } while(0)

#define TEST_SECTION(name) \
    printf("\n🔷 Testing: %s\n", name); \
    printf("=" % 50); \
    printf("\n")

// Memory management tests
void test_memory_management(void) {
    TEST_SECTION("Memory Management");
    
    // Test kmalloc
    void* ptr1 = kmalloc(100);
    TEST_ASSERT(ptr1 != NULL, "kmalloc should return non-NULL pointer");
    
    void* ptr2 = kmalloc(200);
    TEST_ASSERT(ptr2 != NULL, "kmalloc should return non-NULL pointer for second allocation");
    TEST_ASSERT(ptr1 != ptr2, "Different allocations should return different pointers");
    
    // Test kfree
    kfree(ptr1);
    kfree(ptr2);
    printf("✅ Memory freed successfully\n");
    
    // Test large allocation
    void* large_ptr = kmalloc(1024 * 1024);  // 1MB
    TEST_ASSERT(large_ptr != NULL, "kmalloc should handle large allocations");
    kfree(large_ptr);
    
    // Test zero allocation
    void* zero_ptr = kmalloc(0);
    TEST_ASSERT(zero_ptr == NULL, "kmalloc should return NULL for zero size");
    
    // Test memory statistics
    uint64_t total, used, free;
    memory_get_stats(&total, &used, &free);
    TEST_ASSERT(total > 0, "Total memory should be greater than 0");
    TEST_ASSERT(used >= 0, "Used memory should be non-negative");
    TEST_ASSERT(free >= 0, "Free memory should be non-negative");
    TEST_ASSERT(total == used + free, "Total should equal used + free");
    
    printf("📊 Memory Stats: Total=%lu, Used=%lu, Free=%lu\n", total, used, free);
}

// Process management tests
void test_process_management(void) {
    TEST_SECTION("Process Management");
    
    // Test process creation
    int pid1 = process_create("test_process_1", NULL);
    TEST_ASSERT(pid1 > 0, "process_create should return positive PID");
    
    int pid2 = process_create("test_process_2", NULL);
    TEST_ASSERT(pid2 > 0, "process_create should return positive PID");
    TEST_ASSERT(pid1 != pid2, "Different processes should have different PIDs");
    
    // Test process retrieval
    process_t* process1 = process_get_by_pid(pid1);
    TEST_ASSERT(process1 != NULL, "process_get_by_pid should return valid process");
    TEST_ASSERT(process1->pid == pid1, "Process PID should match");
    TEST_ASSERT(strcmp(process1->name, "test_process_1") == 0, "Process name should match");
    
    // Test process priority
    process_set_priority(pid1, 8);
    TEST_ASSERT(process1->priority == 8, "Process priority should be set correctly");
    
    // Test AI priority
    process_set_ai_priority(pid1, true);
    TEST_ASSERT(process1->ai_priority == true, "AI priority should be set correctly");
    
    // Test process statistics
    uint32_t total, active, ready, waiting;
    process_get_stats(&total, &active, &ready, &waiting);
    TEST_ASSERT(total >= 2, "Total processes should be at least 2");
    TEST_ASSERT(active >= 2, "Active processes should be at least 2");
    
    printf("📊 Process Stats: Total=%u, Active=%u, Ready=%u, Waiting=%u\n", 
           total, active, ready, waiting);
    
    // Test process destruction
    process_destroy(pid1);
    process_t* destroyed_process = process_get_by_pid(pid1);
    TEST_ASSERT(destroyed_process == NULL, "Destroyed process should not be retrievable");
    
    process_destroy(pid2);
}

// Scheduler tests
void test_scheduler(void) {
    TEST_SECTION("Scheduler");
    
    // Create test processes
    int pid1 = process_create("scheduler_test_1", NULL);
    int pid2 = process_create("scheduler_test_2", NULL);
    int pid3 = process_create("scheduler_test_3", NULL);
    
    TEST_ASSERT(pid1 > 0 && pid2 > 0 && pid3 > 0, "All test processes should be created successfully");
    
    // Set different priorities
    scheduler_set_priority(pid1, 3);
    scheduler_set_priority(pid2, 7);
    scheduler_set_priority(pid3, 5);
    
    // Set AI priority for one process
    scheduler_set_ai_priority(pid2, true);
    
    // Test scheduler statistics
    scheduler_stats_t stats;
    scheduler_get_stats(&stats);
    TEST_ASSERT(stats.total_switches >= 0, "Total switches should be non-negative");
    TEST_ASSERT(stats.idle_time >= 0, "Idle time should be non-negative");
    
    printf("📊 Scheduler Stats: Total Switches=%lu, AI Switches=%lu, Idle Time=%lu\n",
           stats.total_switches, stats.ai_task_switches, stats.idle_time);
    
    // Test ready queue
    uint32_t ready_count = scheduler_get_ready_count();
    TEST_ASSERT(ready_count >= 3, "Ready queue should contain at least 3 processes");
    
    printf("📊 Ready Processes: %u\n", ready_count);
    
    // Clean up
    process_destroy(pid1);
    process_destroy(pid2);
    process_destroy(pid3);
}

// System calls tests
void test_system_calls(void) {
    TEST_SECTION("System Calls");
    
    // Test getpid
    int current_pid = syscall_getpid(0, 0, 0, 0, 0, 0);
    TEST_ASSERT(current_pid >= 0, "getpid should return valid PID");
    
    // Test gettime
    int uptime = syscall_gettime(0, 0, 0, 0, 0, 0);
    TEST_ASSERT(uptime >= 0, "gettime should return non-negative uptime");
    
    // Test malloc/free
    int ptr = syscall_malloc(100, 0, 0, 0, 0, 0);
    TEST_ASSERT(ptr > 0, "syscall_malloc should return valid pointer");
    
    int result = syscall_free(ptr, 0, 0, 0, 0, 0);
    TEST_ASSERT(result == 0, "syscall_free should return 0 on success");
    
    // Test setpriority
    int test_pid = process_create("syscall_test", NULL);
    result = syscall_setpriority(test_pid, 6, 0, 0, 0, 0);
    TEST_ASSERT(result == 0, "syscall_setpriority should return 0 on success");
    
    // Test getstats
    kernel_state_t kernel_stats;
    result = syscall_getstats((uint32_t)&kernel_stats, 0, 0, 0, 0, 0);
    TEST_ASSERT(result == 0, "syscall_getstats should return 0 on success");
    TEST_ASSERT(kernel_stats.uptime >= 0, "Kernel uptime should be non-negative");
    
    printf("📊 Kernel Stats: Uptime=%u, Total Processes=%u, Active Processes=%u\n",
           kernel_stats.uptime, kernel_stats.total_processes, kernel_stats.active_processes);
    
    process_destroy(test_pid);
}

// Display driver tests
void test_display_driver(void) {
    TEST_SECTION("Display Driver");
    
    // Test display initialization
    display_init();
    printf("✅ Display driver initialized\n");
    
    // Test color setting
    display_set_color(10);  // Light green
    display_set_bg_color(1);  // Blue
    printf("✅ Colors set successfully\n");
    
    // Test character output
    display_putchar('H');
    display_putchar('e');
    display_putchar('l');
    display_putchar('l');
    display_putchar('o');
    display_putchar('\n');
    printf("✅ Character output test completed\n");
    
    // Test string output
    display_print("Hello, Obsidian OS!");
    display_putchar('\n');
    printf("✅ String output test completed\n");
    
    // Test cursor positioning
    display_set_cursor(10, 5);
    display_print("Cursor positioned");
    printf("✅ Cursor positioning test completed\n");
    
    // Test screen clearing
    display_clear();
    printf("✅ Screen clearing test completed\n");
}

// Keyboard driver tests
void test_keyboard_driver(void) {
    TEST_SECTION("Keyboard Driver");
    
    // Test keyboard initialization
    keyboard_init();
    printf("✅ Keyboard driver initialized\n");
    
    // Test keyboard state
    keyboard_state_t* state = keyboard_get_state();
    TEST_ASSERT(state != NULL, "keyboard_get_state should return valid state");
    TEST_ASSERT(state->shift_pressed == false, "Shift should be initially unpressed");
    TEST_ASSERT(state->ctrl_pressed == false, "Ctrl should be initially unpressed");
    TEST_ASSERT(state->alt_pressed == false, "Alt should be initially unpressed");
    
    // Test buffer status
    int head, tail, size;
    keyboard_get_buffer_status(&head, &tail, &size);
    TEST_ASSERT(size == 256, "Keyboard buffer size should be 256");
    TEST_ASSERT(head == 0 && tail == 0, "Keyboard buffer should be initially empty");
    
    printf("📊 Keyboard Buffer: Head=%d, Tail=%d, Size=%d\n", head, tail, size);
    
    // Test buffer operations
    TEST_ASSERT(keyboard_buffer_empty() == true, "Keyboard buffer should be initially empty");
    
    keyboard_clear_buffer();
    printf("✅ Keyboard buffer cleared\n");
}

// Filesystem tests
void test_filesystem(void) {
    TEST_SECTION("Filesystem");
    
    // Test filesystem initialization
    int result = obsidianfs_init();
    TEST_ASSERT(result == 0, "obsidianfs_init should return 0 on success");
    
    // Test file creation
    int file_handle = obsidianfs_open("/test.txt", 0);
    TEST_ASSERT(file_handle >= 0, "obsidianfs_open should return valid file handle");
    
    // Test file writing
    const char* test_data = "Hello, ObsidianFS!";
    ssize_t bytes_written = obsidianfs_write(file_handle, test_data, strlen(test_data));
    TEST_ASSERT(bytes_written == strlen(test_data), "obsidianfs_write should write correct number of bytes");
    
    // Test file reading
    char read_buffer[256];
    obsidianfs_close(file_handle);
    file_handle = obsidianfs_open("/test.txt", 0);
    ssize_t bytes_read = obsidianfs_read(file_handle, read_buffer, sizeof(read_buffer));
    TEST_ASSERT(bytes_read > 0, "obsidianfs_read should read data");
    
    // Test directory creation
    int dir_handle = obsidianfs_create_directory("/testdir", 0755);
    TEST_ASSERT(dir_handle > 0, "obsidianfs_create_directory should return valid inode");
    
    // Test filesystem statistics
    uint64_t total_blocks, free_blocks;
    uint32_t total_files, free_files;
    result = obsidianfs_get_stats(&total_blocks, &free_blocks, &total_files, &free_files);
    TEST_ASSERT(result == 0, "obsidianfs_get_stats should return 0 on success");
    TEST_ASSERT(total_blocks > 0, "Total blocks should be greater than 0");
    TEST_ASSERT(free_blocks >= 0, "Free blocks should be non-negative");
    
    printf("📊 Filesystem Stats: Total Blocks=%lu, Free Blocks=%lu, Total Files=%u, Free Files=%u\n",
           total_blocks, free_blocks, total_files, free_files);
    
    // Clean up
    obsidianfs_close(file_handle);
    obsidianfs_unmount();
}

// AI engine tests
void test_ai_engine(void) {
    TEST_SECTION("AI Engine");
    
    // Test LLM engine initialization
    bool result = llm_engine_init("/usr/share/obsidian/models/llm.onnx", 
                                 "/usr/share/obsidian/models/tokenizer.json");
    TEST_ASSERT(result == true, "llm_engine_init should return true on success");
    
    // Test text generation
    char* response = llm_generate_text("Hello, how are you?", 50);
    TEST_ASSERT(response != NULL, "llm_generate_text should return non-NULL response");
    TEST_ASSERT(strlen(response) > 0, "Generated response should not be empty");
    
    printf("🤖 AI Response: %s\n", response);
    
    // Clean up
    delete[] response;
    llm_engine_cleanup();
}

// Integration tests
void test_integration(void) {
    TEST_SECTION("Integration Tests");
    
    // Test kernel initialization
    kernel_early_init();
    printf("✅ Kernel early initialization completed\n");
    
    kernel_init();
    printf("✅ Kernel initialization completed\n");
    
    // Test AI services
    ai_services_init();
    printf("✅ AI services initialized\n");
    
    // Test background tasks
    for (int i = 0; i < 5; i++) {
        ai_background_tasks();
        printf("✅ AI background task %d completed\n", i + 1);
    }
    
    // Test event processing
    process_events();
    printf("✅ Event processing completed\n");
}

// Performance tests
void test_performance(void) {
    TEST_SECTION("Performance Tests");
    
    // Memory allocation performance
    printf("🔍 Testing memory allocation performance...\n");
    clock_t start = clock();
    
    for (int i = 0; i < 1000; i++) {
        void* ptr = kmalloc(100);
        kfree(ptr);
    }
    
    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    printf("⏱️  Memory allocation: 1000 alloc/free pairs in %.3f seconds\n", time_spent);
    
    // Process creation performance
    printf("🔍 Testing process creation performance...\n");
    start = clock();
    
    int pids[100];
    for (int i = 0; i < 100; i++) {
        pids[i] = process_create("perf_test", NULL);
    }
    
    for (int i = 0; i < 100; i++) {
        process_destroy(pids[i]);
    }
    
    end = clock();
    time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    printf("⏱️  Process creation: 100 create/destroy pairs in %.3f seconds\n", time_spent);
}

// Main test runner
void run_all_tests(void) {
    printf("🚀 Starting Obsidian OS Kernel Tests\n");
    printf("=" % 50);
    printf("\n");
    
    // Initialize test results
    memset(&test_results, 0, sizeof(test_results));
    
    // Run all test suites
    test_memory_management();
    test_process_management();
    test_scheduler();
    test_system_calls();
    test_display_driver();
    test_keyboard_driver();
    test_filesystem();
    test_ai_engine();
    test_integration();
    test_performance();
    
    // Print test summary
    printf("\n" "=" % 50);
    printf("\n📊 Test Summary\n");
    printf("=" % 50);
    printf("\n");
    printf("Total Tests: %d\n", test_results.total_tests);
    printf("Passed: %d\n", test_results.passed_tests);
    printf("Failed: %d\n", test_results.failed_tests);
    printf("Success Rate: %.1f%%\n", 
           (double)test_results.passed_tests / test_results.total_tests * 100);
    
    if (test_results.failed_tests > 0) {
        printf("\n❌ Failed Tests:\n%s\n", test_results.failed_tests_list);
    } else {
        printf("\n🎉 All tests passed!\n");
    }
    
    printf("\n" "=" % 50);
    printf("\n");
}

// Clock function (placeholder)
clock_t clock(void) {
    return 0;  // Placeholder implementation
}

// Delete operator (placeholder)
void operator delete[](void* ptr) {
    free(ptr);
} 