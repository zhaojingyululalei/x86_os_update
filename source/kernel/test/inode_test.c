#include "fs/path.h"
#include "printk.h"
#include "fs/minix_inode.h"
#include "fs/fs.h"
#include "string.h"
#include "fs/minix_entry.h"
#include "fs/mount.h"
/**
 * @brief inode读写测试
 */
static void inode_api_test(void)
{
    minix_inode_desc_t* root_inode = get_root_inode();
    tree_inode(root_inode,2);
    //打开已有文件
    minix_inode_desc_t* test2_inode = minix_inode_open("/test_dir/test2",0,0777);
    ls_inode(test2_inode);
    //打开新文件，没有就创建
    minix_inode_desc_t* hello_inode = minix_inode_open("/test_dir/test2/hello.txt",O_CREAT,0777);
    ls_inode(test2_inode);
    stat_inode(hello_inode);

    //inode读写测试
    minix_inode_truncate(hello_inode,1026);
    // 准备测试数据
    char write_data[1026];
    for (int i = 0; i < 1026; i++) {
        write_data[i] = (char)(i % 256); // 填充 0-255 的循环数据
    }

    // 写入数据
    int write_size = minix_inode_write(hello_inode, write_data, sizeof(write_data), 0, sizeof(write_data));
    if (write_size != sizeof(write_data)) {
        dbg_info("Failed to write data to hello.txt\r\n");
        return;
    }

    // 读取数据
    char read_data[1026];
    int read_size = minix_inode_read(hello_inode, read_data, sizeof(read_data), 0, sizeof(read_data));
    if (read_size != sizeof(read_data)) {
        dbg_info("Failed to read data from hello.txt\r\n");
        return;
    }

    // 验证数据
    if (strncmp(write_data, read_data, sizeof(write_data)) == 0) {
        dbg_info("Data verification passed: write and read data match!\r\n");
    } else {
        dbg_info("Data verification failed: write and read data do not match!\r\n");
    }
}
/**
 * @brief 目录增删改查测试
 */
static void inode_entry_test(void)
{
    minix_inode_desc_t* root_inode = get_root_inode();
    tree_inode(root_inode,2);
    minix_inode_desc_t* test_dir_inode = minix_inode_open("/test_dir",0,0777);
    ls_inode(test_dir_inode);
    //打开已有文件
    minix_inode_desc_t* test2_inode = minix_inode_open("/test_dir/test2",0,0777);
    ls_inode(test2_inode);

    minix_mkdir("/test_dir/test2/hello",0777);
    ls_inode(test2_inode);

    minix_rmdir("/test_dir/test2/hello");
    ls_inode(test2_inode);

    delete_entry_dir(test_dir_inode,"test2",true);
    ls_inode(test_dir_inode);

}
static void inode_link_test(void)
{
    minix_inode_desc_t* root_inode = get_root_inode();
    tree_inode(root_inode,2);

    minix_link("/test_dir/test2/test2.txt","/test_dir/cp_test2.txt");
    minix_inode_desc_t* test_dir_inode = minix_inode_open("/test_dir",0,0777);
    ls_inode(test_dir_inode);
    tree_inode(root_inode,2);

    minix_unlink("/test_dir/cp_test2.txt");
    ls_inode(test_dir_inode);
    tree_inode(root_inode,2);

    minix_unlink("/test_dir/test2/test2.txt");
    ls_inode(test_dir_inode);
    tree_inode(root_inode,2);
}
/** */
static inode_mount_test(void)
{
    minix_inode_desc_t* root_inode = get_root_inode();
    tree_inode(root_inode,2);

    sys_mount("/test_dir/mnt",7,17,FS_MINIX);

    minix_inode_desc_t* mnt_inode = minix_inode_open("/test_dir/mnt",0,0777);
    ls_inode(mnt_inode);
    tree_inode(mnt_inode,2);

    minix_inode_desc_t* test_dir_inode = minix_inode_open("/test_dir",0,0777);
    ls_inode(test_dir_inode);
    tree_inode(root_inode,2);
    
    minix_inode_open("/test_dir/mnt/world.txt",O_CREAT,0777);
    ls_inode(mnt_inode);
    tree_inode(root_inode,2);

    minix_mkdir("/test_dir/mnt/hello",0777);
    ls_inode(mnt_inode);
    tree_inode(mnt_inode,2);

    minix_rmdir("/test_dir/mnt/hello");
    ls_inode(mnt_inode);
    //tree_inode(mnt_inode,2);
    tree_inode(root_inode,2);

    sys_unmount("/test_dir/mnt");
    tree_inode(root_inode,2);
}
void inode_test(void)
{
   
    // 不需要规范化的路径
    ASSERT(strcmp(path_normalize("/test_dirtest2test2.txt"), "/test_dirtest2test2.txt") == 0);
    ASSERT(strcmp(path_normalize("filename.txt"), "filename.txt") == 0);

    // 需要规范化的路径
    ASSERT(strcmp(path_normalize("/test_dir/./test2//test2.txt"), "/test_dir/test2/test2.txt") == 0);
    ASSERT(strcmp(path_normalize("/a/b/../c"), "/a/c") == 0);
    ASSERT(strcmp(path_normalize("/"), "/") == 0);
    ASSERT(strcmp(path_normalize("a/b/../../c"), "c") == 0);
    
    
    return;
}
