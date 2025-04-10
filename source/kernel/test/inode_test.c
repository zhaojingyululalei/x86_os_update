#include "fs/buffer.h"
#include "printk.h"
#include "dev/dev.h"
#include "dev/ide.h"
#include "fs/minix_fs.h"
#include "mem/kmalloc.h"
#include "fs/stat.h"
#include "string.h"
static void inode_opts_test(void)
{
    inode_t inode;
    get_dev_inode(&inode, 7, 16, 1);
    minix_inode_t *inode_data = inode.data;
    buffer_t *bufa = fs_read_to_buffer(7, 16, inode_data->zone[0], false);
    minix_dentry_t *dir = bufa->data;

    while (dir->nr)
    {
        dbg_info("dir name = %s\r\n", dir->name);
        dir++;
    }
    inode_get_block(&inode, 8, true);
}
#define READ_BUF_SIZE (1024*5 )
static char read_buf[READ_BUF_SIZE];
#define WRITE_BUF_SIZE (1024*5)
static char write_buf[READ_BUF_SIZE];
void inode_test(void)
{
    inode_t inode;
    get_dev_inode(&inode, 7, 16, 3);
    minix_inode_t *inode_data = inode.data;
    print_mode(inode.data->mode);
    dbg_info("file size:%d\r\n", inode_data->size);
    //读取测试
    if (ISFILE(inode.data->mode))
    {
        read_content_from_izone(&inode,read_buf,READ_BUF_SIZE,0,inode.data->size);
        dbg_info("content is :%s\r\n",read_buf+4*1024+512);
    }
    else if(ISDIR(inode.data->mode))
    {
        minix_dentry_t entry;

        for (int i = 0; i < inode.data->size / sizeof(minix_dentry_t); i++)
        {
            int offset = i * sizeof(minix_dentry_t);
            int read_len = sizeof(minix_dentry_t);
            read_content_from_izone(&inode, &entry, sizeof(minix_dentry_t), offset, read_len);
            if (entry.nr)
            {
                dbg_info("entry  name = %s\r\n", entry.name);
            }
        }
    }
    //写测试
    if (ISFILE(inode_data->mode)) {
        // 1. 填充写入缓冲区（示例：写入字符串 + 随机数据）
        const char *test_str = "Hello, Minix FS! Writing test data.\r\n";
        strcpy(write_buf, test_str);
        
        // 填充剩余缓冲区（可选）
        for (int i = strlen(test_str); i < WRITE_BUF_SIZE; i++) {
            write_buf[i] = (i % 26) + 'A';  // 填充A-Z循环
        }

        // 2. 写入数据到文件
        int write_size = WRITE_BUF_SIZE;  // 尝试写入5KB
        int write_offset = READ_BUF_SIZE;             // 从文件开头写入（可修改为追加模式）

        int ret = write_content_to_izone(&inode, write_buf, write_size, write_offset,write_size);
        if (ret < 0) {
            dbg_info("write failed! error: %d\r\n", ret);
        } else {
            dbg_info("write success! bytes written: %d\r\n", ret);
            
            // 3. 验证写入结果（重新读取）
            memset(read_buf, 0, READ_BUF_SIZE);
            read_content_from_izone(&inode, read_buf, READ_BUF_SIZE, write_offset, write_size);
            if(strncmp(read_buf,write_buf,write_size)==0){
                dbg_info("write success\r\n");
            }else{
                dbg_info("write fail\r\n");
            }
        }
    } 
    else if (ISDIR(inode_data->mode)) {
        // 目录写入测试：创建一个新目录项
        minix_dentry_t new_entry;
        new_entry.nr = 123;  // 假设目标inode号
        strncpy(new_entry.name, "testfile.txt", MINIX1_NAME_LEN);

        // 追加写入到目录末尾
        int offset = inode_data->size;  // 追加到目录末尾
        int ret = write_content_to_izone(&inode, &new_entry, sizeof(minix_dentry_t), offset,sizeof(minix_dentry_t));
        if (ret == sizeof(minix_dentry_t)) {
            dbg_info("directory entry added!\r\n");
        } else {
            dbg_info("failed to add directory entry!\r\n");
        }
    }
}
