#include "fs/minix/minix_supper.h"
#include "mem/kmalloc.h"
#include "fs/super.h"
#include "fs/buffer.h"
#include "printk.h"
#include "fs/minix/minix_cfg.h"
#include "dev/dev.h"
#include "dev/ide.h"
/**
 * @brief 分配一个超级快，并加入链表
 */ 
static minix_super_t *minix_super_alloc(void)
{
    minix_super_t *super = (minix_super_t *)kmalloc(sizeof(minix_super_t));
    list_t *super_list = get_super_list();
    list_insert_last(super_list, &super->base.node);
}

/**
 * @brief 释放一个超级块，并从链表中移除
 */
void minix_super_free(minix_super_t *super)
{
    list_t *super_list = get_super_list();
    list_remove(super_list, &super->base.node);
    kfree(super);
}

/**
 * @brief 通过设备号，查找相应的超级块是否在链表中并返回
 */
static minix_super_t *minix_super_find(dev_t dev)
{
    list_node_t *cur = get_super_list()->first;
    while (cur)
    {
        super_block_t *super = list_node_parent(cur, super_block_t, node);
        if (super->dev == dev)
        {
            return (minix_super_t *)super;
        }
        cur = cur->next;
    }
    return NULL;
}
/**
 * @brief 从磁盘中读取超级块，并保存到data
 */
static int minix_super_read(minix_super_t *super, dev_t dev)
{
    int major = MAJOR(dev);
    int minor = MINOR(dev);
    buffer_t *super_buf = fs_read_to_buffer(major, minor, 1, true);
    if (!super_buf)
    {
        dbg_error("Failed to read super block from disk!!!\r\n");
        return -1;
    }
    super->data = super_buf->data;
    if (super->data->magic != MINIX1_MAGIC)
    {
        dbg_error("minix fs root super block get failz!!!\r\n");
        return -2;
    }
    buffer_t *imap_buf = fs_read_to_buffer(major, minor, 2, true);
    super->inode_map = (uint8_t *)imap_buf->data;

    buffer_t *zmap_buf = fs_read_to_buffer(major, minor, 2 + super->data->imap_blocks, true);
    super->zone_map = (uint8_t *)zmap_buf->data;
    return 0;
}
/**
 * @brief 获取磁盘中的超级块信息，并填充扩展信息
 */
minix_super_t *minix_get_super(dev_t dev)
{
    int ret;
    int major = MAJOR(dev);
    int minor = MINOR(dev);
    minix_super_t *super;
    super = minix_super_find(dev);

    if (super)
    {
        // 即使超级块曾经读取到了链表当中，为了维护缓存一致性，再从读取一遍
        ret = minix_super_read(super,dev);
        ASSERT(ret >=0);
        return super;
    }
    // 如果不存在，就从磁盘中获取，并加入超级块链表
    super = minix_super_alloc();
    ret = minix_super_read(super,dev);
    ASSERT(ret >= 0);

    super->base.dev = dev;
    fs_type_t type = dev_control(dev,PART_CMD_FS_TYPE,0,0);
    super->base.type = type;
    return super;
}


