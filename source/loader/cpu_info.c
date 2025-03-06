#include "cpu_info.h"
#include "cpu_instr.h"
#include "printk32.h"
extern void get_cpu_manufacture_info(void);
extern uint32_t cpu_info;
// 获取 CPU 的核心信息和 APIC ID
void get_cpu_info(cpu_info_t *info) {
    int max_leaf, eax,ebx, ecx, edx;
    int level, apic_id;
    int logical_cores = 0;
    int physical_cores = 0;

    info->apic_ids_max = -1; //无效APIC ID

    // 获取 CPUID 的最大叶子节点
    asm volatile("cpuid"
                 : "=a"(max_leaf)
                 : "a"(0x0)
                 : "ebx", "ecx", "edx");

    // 检查是否支持扩展拓扑枚举（叶子节点 0x0B）
    if (max_leaf < 0x0B) {
        printk("CPUID leaf 0x0B not supported!\n");
        return;
    }

    // 遍历扩展拓扑枚举的子叶子节点
    for (level = 0; ; level++) {
        asm volatile(
            "cpuid\n\t"
            :"=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
            :"a"(0x0B), "c"(level)
        );
       

        // 获取当前级别的类型（ECX[15:8]）
        int level_type = (ecx >> 8) & 0xFF;

        if (level_type == 0) {
            // 级别类型为 0，表示枚举结束
            break;
        }

        // 获取当前级别的核心数（EAX[15:0]）
        int cores_at_level = eax & 0xFFFF;

        if (level_type == 1) {
            // 级别类型为 1，表示逻辑核心
            logical_cores = cores_at_level;
        } else if (level_type == 2) {
            // 级别类型为 2，表示物理核心
            physical_cores = cores_at_level;
        }

        // 获取当前核心的 APIC ID（EBX[31:0]）
        apic_id = ebx;

        // 保存 APIC ID
        info->apic_ids_max = apic_id;
    }

    // 保存核心数
    info->physical_cores = physical_cores;
    info->logical_cores = logical_cores;

    get_cpu_manufacture_info();
    info->manufacture = &cpu_info;
}