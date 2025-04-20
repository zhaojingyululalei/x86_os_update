// #include "dev/ide.h"
// #include "dev/dev.h"
// #include "mem/kmalloc.h"
// #include "printk.h"
// void ide_test(void){
//     dev_show_all();
//     int sector_cnt = 16;
//     uint8_t *write_buf = kmalloc(SECTOR_SIZE * sector_cnt);
//     uint8_t *read_buf = kmalloc(SECTOR_SIZE * sector_cnt);
//     for (int i = 0; i < SECTOR_SIZE * 2; i++)
//     {
//         if (i % 2 == 0)
//         {
//             write_buf[i] = '0x66';
//         }
//         else
//         {
//             write_buf[i] = '0xbb';
//         }
//     }

//     int devfd = dev_open(6,3,0);
//     dev_write(devfd,0,write_buf,SECTOR_SIZE * sector_cnt,0);
//     dev_read(devfd,0,read_buf,sector_cnt*SECTOR_SIZE,0);
//     int ret = strncmp(write_buf, read_buf, SECTOR_SIZE * sector_cnt);
//     if (ret == 0)
//     {
//         dbg_info("dma read write successful\r\n");
//     }
//     else
//     {
//         dbg_info("dma read write failed\r\n");
//     }
//     kfree(write_buf);
//     kfree(read_buf);
// }