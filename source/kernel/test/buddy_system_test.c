#include "mem/buddy_system.h"
#include "printk.h"
static buddy_system_t buddy_allocator;

void fix_buddy_system_test(void){
    // 初始化伙伴系统，管理 4KB ~ 36KB 的内存空间
    buddy_system_init(&buddy_allocator, 4 * 1024, 9 * 4 * 1024);

    dbg_info("Buddy System Test Start:\r\n");

    // 测试 1: 分配 8KB 内存
    void *ptr1 = buddy_system_alloc(&buddy_allocator, 8 * 1024);
    if (ptr1) {
        dbg_info("Allocated 8KB at address: %x\r\n", ptr1);
    } else {
        dbg_info("Failed to allocate 8KB\r\n");
    }

    // 测试 2: 分配 4KB 内存
    void *ptr2 = buddy_system_alloc(&buddy_allocator, 4 * 1024);
    if (ptr2) {
        dbg_info("Allocated 4KB at address: %x\r\n", ptr2);
    } else {
        dbg_info("Failed to allocate 4KB\r\n");
    }

    // 测试 3: 分配 16KB 内存
    void *ptr3 = buddy_system_alloc(&buddy_allocator, 16 * 1024);
    if (ptr3) {
        dbg_info("Allocated 16KB at address: %x\r\n", ptr3);
    } else {
        dbg_info("Failed to allocate 16KB\r\n");
    }

    // 测试 4: 释放 8KB 内存
    if (ptr1) {
        buddy_system_free(&buddy_allocator, ptr1);
        dbg_info("Freed 8KB at address: %x\r\n", ptr1);
    }

    // 测试 5: 分配 12KB 内存
    void *ptr4 = buddy_system_alloc(&buddy_allocator, 12 * 1024);
    if (ptr4) {
        dbg_info("Allocated 12KB at address: %x\r\n", ptr4);
    } else {
        dbg_info("Failed to allocate 12KB\r\n");
    }

    // 测试 6: 释放 4KB 内存
    if (ptr2) {
        buddy_system_free(&buddy_allocator, ptr2);
        dbg_info("Freed 4KB at address: %x\r\n", ptr2);
    }

    // 测试 7: 释放 16KB 内存
    if (ptr3) {
        buddy_system_free(&buddy_allocator, ptr3);
        dbg_info("Freed 16KB at address: %x\r\n", ptr3);
    }

    // 测试 8: 释放 12KB 内存
    if (ptr4) {
        buddy_system_free(&buddy_allocator, ptr4);
        dbg_info("Freed 12KB at address: %x\r\n", ptr4);
    }

    dbg_info("Buddy System Test End.\r\n");
}
void buddy_system_test(void) {
    // 初始化伙伴系统，管理 4KB ~ 36KB 的内存空间
    buddy_dynamic_system_init(&buddy_allocator, 4 * 1024, 8 * 4 * 1024);

    dbg_info("Buddy System Test Start:\r\n");

    // 测试 1: 分配 8KB 内存
    void *ptr1 = buddy_system_dynamic_alloc(&buddy_allocator, 8 * 1024-4);
    if (ptr1) {
        dbg_info("Allocated 8KB at address: %x\r\n", ptr1);
    } else {
        dbg_info("Failed to allocate 8KB\r\n");
    }

    // 测试 2: 分配 4KB 内存
    void *ptr2 = buddy_system_dynamic_alloc(&buddy_allocator, 4 * 1024-4);
    if (ptr2) {
        dbg_info("Allocated 4KB at address: %x\r\n", ptr2);
    } else {
        dbg_info("Failed to allocate 4KB\r\n");
    }

    // 测试 3: 分配 16KB 内存
    void *ptr3 = buddy_system_dynamic_alloc(&buddy_allocator, 16 * 1024-4);
    if (ptr3) {
        dbg_info("Allocated 16KB at address: %x\r\n", ptr3);
    } else {
        dbg_info("Failed to allocate 16KB\r\n");
    }

    // 测试 4: 释放 8KB 内存
    if (ptr1) {
        buddy_system_dynamic_free(&buddy_allocator, ptr1);
        dbg_info("Freed 8KB at address: %x\r\n", ptr1);
    }

    // 测试 5: 分配 12KB 内存
    void *ptr4 = buddy_system_dynamic_alloc(&buddy_allocator, 12 * 1024-4);
    if (ptr4) {
        dbg_info("Allocated 12KB at address: %x\r\n", ptr4);
    } else {
        dbg_info("Failed to allocate 12KB\r\n");
    }

    // 测试 6: 释放 4KB 内存
    if (ptr2) {
        buddy_system_dynamic_free(&buddy_allocator, ptr2);
        dbg_info("Freed 4KB at address: %x\r\n", ptr2);
    }

    // 测试 7: 释放 16KB 内存
    if (ptr3) {
        buddy_system_dynamic_free(&buddy_allocator, ptr3);
        dbg_info("Freed 16KB at address: %x\r\n", ptr3);
    }

    // 测试 8: 释放 12KB 内存
    if (ptr4) {
        buddy_system_dynamic_free(&buddy_allocator, ptr4);
        dbg_info("Freed 12KB at address: %x\r\n", ptr4);
    }

    dbg_info("Buddy System Test End.\r\n");
}