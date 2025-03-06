#include "apci.h"
#include "printk.h"
void APCI_test(void){
    // 打印保存的信息
    dbg_info("MADT saved at address: 0x%x\r\n", &madt_static);
    dbg_info("Local APIC Address: 0x%08x\r\n", madt_static.local_apic_addr);
    dbg_info("Flags: 0x%08x\n", madt_static.flags);
    dbg_info("MADT Entry Count: %d\r\n", madt_entry_count);

    for (uint32_t i = 0; i < madt_entry_count; i++)
    {
        dbg_info("Entry %d: Type = %d, Length = %d\r\n",
                 i, madt_entries_static[i].header.type, madt_entries_static[i].header.length);
        if(madt_entries_static[i].header.type==MADT_ENTRY_IO_APIC){
            dbg_info("io_apic_addr:0x%x\r\n",madt_entries_static[i].io_apic.io_apic_addr);
        }
    }
}