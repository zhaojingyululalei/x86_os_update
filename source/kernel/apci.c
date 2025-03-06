#include "apci.h"
#include "string.h"
#include "printk.h"
#include "boot_info.h"
extern boot_info_t *os_info;
// 保存 RSDT、MADT 和 MADT 子结构的静态表
rsdt_t rsdt_static;
madt_t madt_static;
madt_entry_t madt_entries_static[MAX_MADT_ENTRIES];
uint32_t madt_entry_count = 0;
/**
 * @brief  计算校验和
 */
static uint8_t calculate_checksum(const void *data, size_t length)
{
    uint8_t sum = 0;
    const uint8_t *ptr = (const uint8_t *)data;
    for (size_t i = 0; i < length; i++)
    {
        sum += ptr[i];
    }
    return sum;
}


/**
 * @brief 在内存中查找 RSDP
 */
rsdp_t *find_rsdp()
{
    for (ph_addr_t addr = RSDP_SEARCH_START; addr < RSDP_SEARCH_END; addr += 16)
    {
        // 读取内存中的签名
        char signature[9] = {0};
        memcpy(signature, addr, 8);

        // 检查签名是否匹配
        if (memcmp(signature, RSDP_SIGNATURE, 8) == 0)
        {
            // 找到签名，读取完整的 RSDP
            rsdp_t *rsdp = (rsdp_t *)addr;

            // 计算校验和
            uint8_t checksum = calculate_checksum(rsdp, rsdp->revision == 0 ? 20 : 36);

            // 验证校验和
            if (checksum == 0)
            {
                return rsdp; // 校验和通过，返回 RSDP
            }
        }
    }

    return NULL; // 未找到有效的 RSDP
}

 /**
  * @brief 遍历 RSDT 并找到 MADT
  */
madt_t *find_madt(rsdt_t *rsdt)
{
    // 计算 RSDT 中的条目数量
    uint32_t entry_count = (rsdt->header.length - sizeof(acpi_table_header_t)) / 4;

    // 遍历 RSDT 的条目
    for (uint32_t i = 0; i < entry_count; i++)
    {
        // 获取条目的物理地址
        uint32_t table_addr = rsdt->entries[i];

        // 将物理地址转换为指针
        acpi_table_header_t *table_header = (acpi_table_header_t *)table_addr;

        // 检查表签名是否为 "APIC"
        if (memcmp(table_header->signature, ACPI_SIGNATURE_MADT, 4) == 0)
        {
            // 找到 MADT
            int calc_ret;
            calc_ret = calculate_checksum(table_header, table_header->length);
            if (calc_ret == 0)
                return (madt_t *)table_header;
        }
    }

    return NULL; // 未找到 MADT
}

/**
 * @brief 解析 MADT 并保存所有子结构
 */
void parse_madt(madt_t *madt)
{
    if (madt == NULL)
    {
        dbg_info("MADT not found.\n");
        return;
    }

    // 保存 MADT 表头信息
    memcpy(&madt_static.header, &madt->header, sizeof(acpi_table_header_t));
    madt_static.local_apic_addr = madt->local_apic_addr;
    madt_static.flags = madt->flags;

    // 遍历 MADT 的子结构
    uint8_t *entry_ptr = madt->entries;
    uint8_t *end_ptr = (uint8_t *)madt + madt->header.length;
    // entry_ptr[1] 是子结构的长度
    while (entry_ptr < end_ptr && madt_entry_count < MAX_MADT_ENTRIES)
    {
        // 保存子结构
        memcpy(&madt_entries_static[madt_entry_count], entry_ptr, entry_ptr[1]);
        madt_entry_count++;

        // 移动到下一个子结构
        entry_ptr += entry_ptr[1];
    }
}
/**
 * @brief 查找rsdt表
 */
rsdt_t *find_rsdt(rsdp_t *rsdp)
{
    rsdt_t *rsdt = (rsdt_t *)rsdp->rsdt_addr;
    int calc_ret;
    calc_ret = calculate_checksum(rsdt, rsdt->header.length);
    if (calc_ret == 0)
        return rsdt;
    else
        return NULL;
}
/**
 * @brief 获取apci各种表的信息，主要是apic的的信息
 */
void apci_init(void)
{
    rsdp_t *rsdp = find_rsdp();
    rsdt_t *rsdt = find_rsdt(rsdp);
    madt_t *madt = find_madt(rsdt);
    parse_madt(madt);
    os_info->cpu_info.local_apic_addr = madt_static.local_apic_addr;
    for (uint32_t i = 0; i < madt_entry_count; i++)
    {

        if (madt_entries_static[i].header.type == MADT_ENTRY_IO_APIC)
        {
            os_info->cpu_info.io_apic_addr = madt_entries_static[i].io_apic.io_apic_addr;
        }
    }
    return 0;
}