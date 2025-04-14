#include "fs/minix_super.h"
#include "string.h"
#include "fs/buffer.h"
#include "printk.h"
static minix_super_desc_t minix_super_blocks[MAX_MINIX_SUPER_BLOCKS];

static minix_super_desc_t *minix_super_desc_alloc(void)
{
    for (int i = 0; i < MAX_MINIX_SUPER_BLOCKS; i++)
    {

        if (minix_super_blocks[i].state == MINIX_SUPER_FREE)
        {
            minix_super_blocks[i].state = MINIX_SUPER_USED;
            return &minix_super_blocks[i];
        }
    }
    dbg_error("minix fs super desc alloc failz!!!\r\n");
    return NULL;
}

static void minix_super_desc_free(minix_super_desc_t *super)
{
    if (!super)
        return;
    memset(super, 0, sizeof(minix_super_desc_t));
    super->state = MINIX_SUPER_FREE;
}

static minix_super_desc_t *minix_super_desc_find(int major, int minor)
{
    for (int i = 0; i < MAX_MINIX_SUPER_BLOCKS; i++)
    {
        if (minix_super_blocks[i].state == MINIX_SUPER_USED && minix_super_blocks[i].major == major && minix_super_blocks[i].minor == minor)
        {
            return &minix_super_blocks[i];
        }
    }
    return NULL;
}
/**
 * @brief 获取某个分区的超级块描述信息
 */
minix_super_desc_t *minix_get_super_desc(int major, int minor)
{
    minix_super_desc_t *super_desc = minix_super_desc_find(major, minor);
    if (!super_desc)
    {
        super_desc = minix_super_desc_alloc();
    }
    if (!super_desc)
    {
        dbg_error("minix fs super desc alloc failz!!!\r\n");
        return NULL;
    }
    // 不管是已经找到的，还是新分配的super_desc,都需要重新读取磁盘中对应的超级快数据，保持数据一致性
    buffer_t *super_buf = fs_read_to_buffer(major, minor, 1, true);
    if (!super_buf)
    {
        dbg_error("Failed to read super block from disk!!!\r\n");
        return NULL;
    }
    minix_super_t *minix_sb = (minix_super_t *)super_buf->data;
    if (minix_sb->magic != MINIX1_MAGIC)
    {
        dbg_error("minix fs root super block get failz!!!\r\n");
        return NULL;
    }
    super_desc->data = minix_sb;
    super_desc->major = major;
    super_desc->minor = minor;

    buffer_t *imap_buf = fs_read_to_buffer(major, minor, 2, true);
    super_desc->inode_map = (uint8_t *)imap_buf->data;

    buffer_t *zmap_buf = fs_read_to_buffer(major, minor, 2 + minix_sb->imap_blocks, true);
    super_desc->zone_map = (uint8_t *)zmap_buf->data;

    return super_desc;
}
/**
 * @brief 释放超级块描述符
 */
int minix_collect_super_desc(int major, int minor)
{
    minix_super_desc_t *super_desc = minix_super_desc_find(major, minor);
    if (super_desc)
    {
        minix_super_desc_free(super_desc);
    }
    else
    {
        dbg_warning("dev major=%d minor=%d not ever mount!!!\r\n", major, minor);
        return -1;
    }
    return 0;
}


