#include "fs/path.h"
#include "printk.h"
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
    //inode_entry_test();


    // const char* path = "/test_dir/test2/test2.txt";
    // inode_t *target = namei(path);
    // minix_inode_t *inode_data = target->data;
    // print_mode(target->data->mode);
    // dbg_info("file size:%d\r\n", inode_data->size);
    // // 读取测试
    // if (ISFILE(target->data->mode))
    // {
    //     memset(read_buf,0,512);
    //     read_content_from_izone(target, read_buf, READ_BUF_SIZE, 0, target->data->size);
    //     dbg_info("content is :%s\r\n", read_buf );
    // }
    // target = named(path);
    // print_mode(target->data->mode);
    // print_entrys(target);
    return;
}
