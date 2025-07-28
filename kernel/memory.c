/**
 * 💠 Obsidian OS Kernel - Memory Management
 * Virtual memory, paging, and memory allocation system
 */

#include "kernel.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Memory management structures
#define PAGE_PRESENT    0x001
#define PAGE_WRITE      0x002
#define PAGE_USER       0x004
#define PAGE_WRITETHROUGH 0x008
#define PAGE_CACHE_DISABLE 0x010
#define PAGE_ACCESSED   0x020
#define PAGE_DIRTY      0x040
#define PAGE_SIZE_4MB   0x080
#define PAGE_GLOBAL     0x100

// Page table entry structure
typedef uint64_t page_table_entry_t;

// Page directory structure
typedef struct {
    page_table_entry_t entries[512];
} page_directory_t;

// Memory block structure for heap management
typedef struct memory_block {
    size_t size;
    bool used;
    struct memory_block* next;
    struct memory_block* prev;
} memory_block_t;

// Global variables
static page_directory_t* kernel_page_directory = NULL;
static void* kernel_heap_start = NULL;
static void* kernel_heap_end = NULL;
static memory_block_t* free_list = NULL;
static uint64_t total_memory = 0;
static uint64_t used_memory = 0;

// Memory map structure
typedef struct {
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
    uint32_t acpi_attributes;
} memory_map_entry_t;

static memory_map_entry_t memory_map[64];
static int memory_map_count = 0;

/**
 * Initialize memory management system
 */
void memory_init(void) {
    // Initialize page directory
    kernel_page_directory = (page_directory_t*)0x1000;
    
    // Clear page directory
    for (int i = 0; i < 512; i++) {
        kernel_page_directory->entries[i] = 0;
    }
    
    // Map kernel space (first 1MB)
    for (int i = 0; i < 256; i++) {
        kernel_page_directory->entries[i] = (i * 0x1000) | PAGE_PRESENT | PAGE_WRITE;
    }
    
    // Map video memory
    kernel_page_directory->entries[0xB8] = 0xB8000 | PAGE_PRESENT | PAGE_WRITE;
    
    // Initialize kernel heap
    kernel_heap_start = (void*)0x100000;  // 1MB
    kernel_heap_end = (void*)0x200000;    // 2MB
    
    // Initialize free list
    free_list = (memory_block_t*)kernel_heap_start;
    free_list->size = (uintptr_t)kernel_heap_end - (uintptr_t)kernel_heap_start - sizeof(memory_block_t);
    free_list->used = false;
    free_list->next = NULL;
    free_list->prev = NULL;
    
    // Load page directory
    __asm__ volatile("mov %0, %%cr3" : : "r"(kernel_page_directory));
    
    // Enable paging
    uint64_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;  // Set PG bit
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
    
    total_memory = 0x200000;  // 2MB for now
    used_memory = 0;
}

/**
 * Allocate memory from kernel heap
 */
void* kmalloc(size_t size) {
    if (size == 0) return NULL;
    
    // Align size to 8 bytes
    size = (size + 7) & ~7;
    
    memory_block_t* current = free_list;
    memory_block_t* best_fit = NULL;
    size_t best_size = (size_t)-1;
    
    // Find best fit
    while (current != NULL) {
        if (!current->used && current->size >= size) {
            if (current->size < best_size) {
                best_fit = current;
                best_size = current->size;
            }
        }
        current = current->next;
    }
    
    if (best_fit == NULL) {
        return NULL;  // Out of memory
    }
    
    // Split block if necessary
    if (best_fit->size > size + sizeof(memory_block_t) + 8) {
        memory_block_t* new_block = (memory_block_t*)((char*)best_fit + sizeof(memory_block_t) + size);
        new_block->size = best_fit->size - size - sizeof(memory_block_t);
        new_block->used = false;
        new_block->next = best_fit->next;
        new_block->prev = best_fit;
        
        if (best_fit->next) {
            best_fit->next->prev = new_block;
        }
        best_fit->next = new_block;
        best_fit->size = size;
    }
    
    best_fit->used = true;
    used_memory += size;
    
    return (void*)((char*)best_fit + sizeof(memory_block_t));
}

/**
 * Free allocated memory
 */
void kfree(void* ptr) {
    if (ptr == NULL) return;
    
    memory_block_t* block = (memory_block_t*)((char*)ptr - sizeof(memory_block_t));
    
    if (!block->used) {
        return;  // Already freed
    }
    
    block->used = false;
    used_memory -= block->size;
    
    // Merge with next block if free
    if (block->next && !block->next->used) {
        block->size += block->next->size + sizeof(memory_block_t);
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
    }
    
    // Merge with previous block if free
    if (block->prev && !block->prev->used) {
        block->prev->size += block->size + sizeof(memory_block_t);
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }
    }
}

/**
 * Allocate a page of memory
 */
void* page_alloc(void) {
    // Find a free page in the page directory
    for (int i = 256; i < 512; i++) {
        if ((kernel_page_directory->entries[i] & PAGE_PRESENT) == 0) {
            // Allocate physical memory
            void* physical_addr = kmalloc(PAGE_SIZE);
            if (physical_addr == NULL) {
                return NULL;
            }
            
            // Map the page
            kernel_page_directory->entries[i] = ((uint64_t)physical_addr) | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
            
            // Invalidate TLB
            __asm__ volatile("invlpg (%0)" : : "r"(i * 0x1000) : "memory");
            
            return (void*)(i * 0x1000);
        }
    }
    
    return NULL;  // No free pages
}

/**
 * Free a page of memory
 */
void page_free(void* addr) {
    uint64_t page_index = ((uint64_t)addr) / 0x1000;
    
    if (page_index >= 512) {
        return;  // Invalid address
    }
    
    page_table_entry_t* entry = &kernel_page_directory->entries[page_index];
    
    if ((*entry & PAGE_PRESENT) == 0) {
        return;  // Page not allocated
    }
    
    // Free physical memory
    void* physical_addr = (void*)(*entry & ~0xFFF);
    kfree(physical_addr);
    
    // Clear page table entry
    *entry = 0;
    
    // Invalidate TLB
    __asm__ volatile("invlpg (%0)" : : "r"(addr) : "memory");
}

/**
 * Get memory statistics
 */
void memory_get_stats(uint64_t* total, uint64_t* used, uint64_t* free) {
    if (total) *total = total_memory;
    if (used) *used = used_memory;
    if (free) *free = total_memory - used_memory;
}

/**
 * Check if address is valid
 */
bool memory_is_valid(void* addr) {
    uint64_t addr_val = (uint64_t)addr;
    
    // Check if address is in kernel space
    if (addr_val < 0x100000 || addr_val >= 0x200000) {
        return false;
    }
    
    // Check if address is page-aligned
    if (addr_val % PAGE_SIZE != 0) {
        return false;
    }
    
    return true;
}

/**
 * Memory copy function
 */
void* memcpy(void* dest, const void* src, size_t n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;
    
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    
    return dest;
}

/**
 * Memory set function
 */
void* memset(void* dest, int c, size_t n) {
    char* d = (char*)dest;
    
    for (size_t i = 0; i < n; i++) {
        d[i] = (char)c;
    }
    
    return dest;
}

/**
 * Memory compare function
 */
int memcmp(const void* ptr1, const void* ptr2, size_t n) {
    const char* p1 = (const char*)ptr1;
    const char* p2 = (const char*)ptr2;
    
    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] - p2[i];
        }
    }
    
    return 0;
} 