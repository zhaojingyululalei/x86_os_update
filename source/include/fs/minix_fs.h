#ifndef __MINIX_FS_H
#define __MINIX_FS_H
#include "types.h"
#include "time/time.h"
#include "cpu_cfg.h"
#define MINIX1_NAME_LEN 14
#define MINIX1_MAGIC 0x137F // 文件系统魔数
//根文件系统所在设备
#define MINIX1FS_ROOT_MAJOR   7
#define MINIX1FS_ROOT_MINOR   16
typedef struct minix_super_t
{
    uint16_t inodes;        // 节点数
    uint16_t zones;         // 逻辑块数
    uint16_t imap_blocks;   // i 节点位图所占用的数据块数
    uint16_t zmap_blocks;   // 逻辑块位图所占用的数据块数
    uint16_t firstdatazone; // 第一个数据逻辑块号
    uint16_t log_zone_size; // log2(每逻辑块数据块数)
    uint32_t max_size;      // 文件最大长度
    uint16_t magic;         // 文件系统魔数
} minix_super_t;
struct _inode_t;
typedef struct _super_block_t{
    minix_super_t* data;//超级快加载到内存中的地址
    uint8_t* inode_map;
    uint8_t* zone_map;
    int major; //记录这是哪个设备的超级快
    int minor;
    struct _inode_t *iroot;       // 根目录 inode
    struct _inode_t *imount;      // 安装到的 inode
}super_block_t;
typedef struct minix_inode_t
{
    uint16_t mode;    // 文件类型和属性(rwx 位)
    uint16_t uid;     // 用户id（文件拥有者标识符）
    uint32_t size;    // 文件大小（字节数）
    uint32_t mtime;   // 修改时间戳 这个时间戳应该用 UTC 时间，不然有瑕疵
    uint8_t gid;      // 组id(文件拥有者所在的组)
    uint8_t nlinks;   // 链接数（多少个文件目录项指向该i 节点）
    uint16_t zone[9]; // 直接 (0-6)、间接(7)或双重间接 (8) 逻辑块号
} minix_inode_t;

typedef struct _inode_t
{
    minix_inode_t* data; //inode地址
    int major;
    int minor; //设备号
    int idx;//inode bitmap下标
    time_t atime; // 访问时间
    time_t mtime; // 修改时间
    time_t ctime; // 创建时间


}inode_t;

// 文件目录项结构
typedef struct minix_dentry_t
{
    uint16_t nr;                    // i 节点
    char name[MINIX1_NAME_LEN]; // 文件名
} minix_dentry_t;

int minix_dblk_alloc(int major, int minor) ;
int minix_dblk_free(int major, int minor, int blk);
int minix_inode_alloc(int major, int minor) ;
int minix_inode_free(int major, int minor, int ino);
int get_dev_super_block(super_block_t* superb,int major,int minor);
super_block_t* get_root_super_block(void);
int get_dev_inode(inode_t* inode, int major, int minor, int idx) ;
inode_t* get_root_inode(void);
int inode_get_block(inode_t* inode, int fblock, bool create);

int read_content_from_izone(inode_t* inode, char* buf, int buf_size, int whence, int read_size);
int write_content_to_izone(inode_t* inode, const char* buf, int buf_size, int whence, int write_size);


int find_entry(inode_t* inode,const char* name,minix_dentry_t* entry);
int add_entry(inode_t *inode, const char *name);
int delete_entry_not_dir(inode_t* inode, const char* name) ;
int delete_entry_dir(inode_t* inode, const char* name, bool recursion);


int print_mode(uint16_t mode);
void print_entrys(inode_t* inode);
#endif