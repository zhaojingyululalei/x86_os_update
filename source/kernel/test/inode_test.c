// #include "fs/minix/minix_supper.h"
// #include "fs/devfs/devfs.h"
// #include "fs/minix/minix_inode.h"
// #include "printk.h"
// #include "string.h"

// void test_disk2_inode(void) {
//     int major = 7;  // 主设备号
//     int minor = 16; // 次设备号
//     dev_t dev = MKDEV(major, minor);
//     inode_opts_t* ops = &minix_inode_opts;

//     // 1. 获取根目录 inode
//     minix_inode_t* root_inode = ops->get_dev_root_inode(dev);
//     if (!root_inode) {
//         dbg_info("Failed to get root inode\n");
//         return;
//     }
//     dbg_info("Root inode opened successfully\n");

//     // 2. 读取根目录内容
//     char root_dir_buf[1024] = {0};
//     int ret = ops->read(root_inode, root_dir_buf, sizeof(root_dir_buf), 0, sizeof(root_dir_buf));
//     if (ret < 0) {
//         dbg_info("Failed to read root directory\n");
//         return;
//     }
//     dbg_info("Root directory content:\n%s\n", root_dir_buf);

//     // 3. 查找目录项 "test_dir"
//     struct dirent found_entry;
//     ret = ops->find_entry(root_inode, "test_dir", &found_entry);
//     if (ret < 0) {
//         dbg_info("Failed to find directory entry 'test_dir'\n");
//         return;
//     }
//     dbg_info("Found directory entry: %s, ino=%d\n", found_entry.name, found_entry.nr);

//     // 4. 添加一个新的目录项 "new_dir"
//     struct dirent new_entry;
//     strcpy(new_entry.name, "new_dir");
//     new_entry.nr = ops->alloc(dev); // 分配一个新的 inode
//     if (new_entry.nr == -1) {
//         dbg_info("Failed to allocate inode for new directory\n");
//         return;
//     }
//     ret = ops->add_entry(root_inode, &new_entry);
//     if (ret < 0) {
//         dbg_info("Failed to add directory entry 'new_dir'\n");
//         return;
//     }
//     dbg_info("Added directory entry: %s, ino=%d\n", new_entry.name, new_entry.nr);

    
//     // 5. 删除刚刚添加的目录项 "new_dir"
//     ret = ops->delete_entry(root_inode, &new_entry);
//     if (ret < 0) {
//         dbg_info("Failed to delete directory entry 'new_dir'\n");
//         return;
//     }
//     dbg_info("Deleted directory entry: %s\n", new_entry.name);

//     // 6. 验证文件内容
//     minix_inode_t* test_file_inode = ops->open(dev, found_entry.nr);
//     if (!test_file_inode) {
//         dbg_info("Failed to open inode for 'test_dir'\n");
//         return;
//     }
//     char file_buf[128] = {0};
//     ret = ops->read(test_file_inode, file_buf, sizeof(file_buf), 0, sizeof(file_buf));
//     if (ret < 0) {
//         dbg_info("Failed to read file content\n");
//         return;
//     }
//     dbg_info("File content in 'test_dir':\n%s\n", file_buf);

//     // 清理
//     ops->destroy(root_inode);
//     ops->destroy(test_file_inode);
//     dbg_info("All tests passed!\n");
// }
// void inode_test(void) {
//     test_disk2_inode();
// }