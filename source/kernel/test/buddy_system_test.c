#include "mem/buddy_system.h"
#include "printk.h"
#define MEMORY_CAPACITY (8 * 4 * 1024)  // 32KB 内存
#define BASE_ADDR (void*)(4*1024) // 基地址
#define EXPAND_ADDR (void*)(64*1024*1024) // 基地址
static buddy_mmpool_fix_t fix_buddy_allocator;
static buddy_mmpool_dyn_t dyn_buddy_allocator;

// 静态伙伴系统测试
void fix_buddy_system_test(void) {
    // 初始化静态伙伴系统
    buddy_mmpool_fix_init(&fix_buddy_allocator, BASE_ADDR, MEMORY_CAPACITY);

    dbg_info("Fixed Buddy System Test Start:\r\n");

    // 测试 1: 分配 8KB 内存
    void *ptr1 = fix_buddy_allocator.alloc(&fix_buddy_allocator.sys, 8 * 1024 - 4);
    if (ptr1) {
        dbg_info("Allocated 8KB at address: 0x%x\r\n", ptr1);
    } else {
        dbg_info("Failed to allocate 8KB\r\n");
    }

    // 测试 2: 分配 4KB 内存
    void *ptr2 = fix_buddy_allocator.alloc(&fix_buddy_allocator.sys, 4 * 1024 - 4);
    if (ptr2) {
        dbg_info("Allocated 4KB at address: 0x%x\r\n", ptr2);
    } else {
        dbg_info("Failed to allocate 4KB\r\n");
    }

    // 测试 3: 分配 16KB 内存
    void *ptr3 = fix_buddy_allocator.alloc(&fix_buddy_allocator.sys, 16 * 1024 - 4);
    if (ptr3) {
        dbg_info("Allocated 16KB at address: 0x%x\r\n", ptr3);
    } else {
        dbg_info("Failed to allocate 16KB\r\n");
    }

    // 释放 8KB
    if (ptr1) {
        fix_buddy_allocator.free(&fix_buddy_allocator.sys, ptr1);
        dbg_info("Freed 8KB at address: 0x%x\r\n", ptr1);
    }

    // 分配 12KB
    void *ptr4 = fix_buddy_allocator.alloc(&fix_buddy_allocator.sys, 12 * 1024 - 4);
    if (ptr4) {
        dbg_info("Allocated 12KB at address: 0x%x\r\n", ptr4);
    } else {
        dbg_info("Failed to allocate 12KB\r\n");
    }

    // 释放 4KB
    if (ptr2) {
        fix_buddy_allocator.free(&fix_buddy_allocator.sys, ptr2);
        dbg_info("Freed 4KB at address: 0x%x\r\n", ptr2);
    }

    // 释放 16KB
    if (ptr3) {
        fix_buddy_allocator.free(&fix_buddy_allocator.sys, ptr3);
        dbg_info("Freed 16KB at address: 0x%x\r\n", ptr3);
    }

    // 释放 12KB
    if (ptr4) {
        fix_buddy_allocator.free(&fix_buddy_allocator.sys, ptr4);
        dbg_info("Freed 12KB at address: 0x%x\r\n", ptr4);
    }

    dbg_info("Fixed Buddy System Test End.\r\n");
}

// 动态伙伴系统测试
void dynamic_buddy_system_test(void) {
    // 初始化动态伙伴系统
    buddy_mmpool_dyn_init(&dyn_buddy_allocator, BASE_ADDR, MEMORY_CAPACITY);
    dyn_buddy_allocator.sbrk(&dyn_buddy_allocator.sys,EXPAND_ADDR,MEMORY_CAPACITY);
    dbg_info("Dynamic Buddy System Test Start:\r\n");

    // 分配 8KB
    void *ptr1 = dyn_buddy_allocator.alloc(&dyn_buddy_allocator.sys, 8 * 1024 - 4);
    if (ptr1) {
        dbg_info("Allocated 8KB at address: 0x%x\r\n", ptr1);
    } else {
        dbg_info("Failed to allocate 8KB\r\n");
    }

    // 分配 4KB
    void *ptr2 = dyn_buddy_allocator.alloc(&dyn_buddy_allocator.sys, 4 * 1024 - 4);
    if (ptr2) {
        dbg_info("Allocated 4KB at address: 0x%x\r\n", ptr2);
    } else {
        dbg_info("Failed to allocate 4KB\r\n");
    }

    // 分配 16KB
    void *ptr3 = dyn_buddy_allocator.alloc(&dyn_buddy_allocator.sys, 16 * 1024 - 4);
    if (ptr3) {
        dbg_info("Allocated 16KB at address: 0x%x\r\n", ptr3);
    } else {
        dbg_info("Failed to allocate 16KB\r\n");
    }

    // 释放 8KB
    if (ptr1) {
        dyn_buddy_allocator.free(&dyn_buddy_allocator.sys, ptr1);
        dbg_info("Freed 8KB at address: 0x%x\r\n", ptr1);
    }

    // 分配 12KB
    void *ptr4 = dyn_buddy_allocator.alloc(&dyn_buddy_allocator.sys, 12 * 1024 - 4);
    if (ptr4) {
        dbg_info("Allocated 12KB at address: 0x%x\r\n", ptr4);
    } else {
        dbg_info("Failed to allocate 12KB\r\n");
    }

    // 释放 4KB
    if (ptr2) {
        dyn_buddy_allocator.free(&dyn_buddy_allocator.sys, ptr2);
        dbg_info("Freed 4KB at address: 0x%x\r\n", ptr2);
    }

    // 释放 16KB
    if (ptr3) {
        dyn_buddy_allocator.free(&dyn_buddy_allocator.sys, ptr3);
        dbg_info("Freed 16KB at address: 0x%x\r\n", ptr3);
    }

    // 释放 12KB
    if (ptr4) {
        dyn_buddy_allocator.free(&dyn_buddy_allocator.sys, ptr4);
        dbg_info("Freed 12KB at address: 0x%x\r\n", ptr4);
    }

    dbg_info("Dynamic Buddy System Test End.\r\n");
}

// 运行测试
void buddy_system_test(void) {
    fix_buddy_system_test();
    dynamic_buddy_system_test();
}