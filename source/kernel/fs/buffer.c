#include "fs/buffer.h"
#include "printk.h"
#include "algrithm.h"
#include "mem/bitmap.h"
#include "task/task.h"
#include "dev/dev.h"
#include "string.h"
#include "cpu_cfg.h"
#include "fs/devfs/devfs.h"
static buffer_t fs_buffer[FS_BUFFER_NR];
static list_t fs_buffer_free_list;
static rb_tree_t fs_buffer_tree;
static mutex_t fs_buf_mutex;
static list_t fs_LRU_list;
/**
 * 比较 `buffer_t` 的 `score`
 */
static int buffer_compare(const void *a, const void *b)
{
    buffer_t *buf_a = (buffer_t *)a;
    buffer_t *buf_b = (buffer_t *)b;

    if (buf_a->major != buf_b->major)
    {
        return buf_a->major - buf_b->major;
    }
    if (buf_a->minor != buf_b->minor)
    {
        return buf_a->minor - buf_b->minor;
    }
    return buf_a->blk - buf_b->blk;
}

/**
 * 获取 `buffer_t` 的 `rb_node_t`
 */
static rb_node_t *buffer_get_node(const void *data)
{
    return &((buffer_t *)data)->rbnode;
}

static buffer_t *node_get_buffer(rb_node_t *node)
{
    buffer_t *s = rb_node_parent(node, buffer_t, rbnode);
    return s;
}

static void buffer_print(buffer_t *s)
{
    dbg_info("buffer major = %d,minor = %d,blk = %d,dirty = %d,valid = %d\r\n",
             s->major, s->minor, s->blk, s->dirty, s->valid);
}
static int compare_key(const void *key, const void *data)
{
    int *arr = (int *)key;
    int major = arr[0];
    int minor = arr[1];
    int blk = arr[2];
    buffer_t *s = (buffer_t *)data;

    if (major != s->major)
    {
        return major - s->major;
    }
    if (minor != s->minor)
    {
        return minor - s->minor;
    }
    return blk - s->blk;
}
/**
 * @brief 定时同步
 */
static void time_sync(void)
{
    while (true)
    {
        sys_sleep(FS_PERIOD_SYNC_TIME_SECOND * 1000);
        sys_sync();
    }
}
void fs_buffer_init(void)
{
    rb_tree_t *tree = &fs_buffer_tree;
    list_t *list = &fs_buffer_free_list;
    list_init(list);
    list_init(&fs_LRU_list);
    rb_tree_init(tree, buffer_compare, buffer_get_node, node_get_buffer, NULL);
    sys_mutex_init(&fs_buf_mutex);
    for (int i = 0; i < FS_BUFFER_NR; i++)
    {
        list_insert_last(list, &fs_buffer[i].lnode);
    }
    create_kernel_task((ph_addr_t)time_sync, "time sync", TASK_PRIORITY_DEFAULT, NULL);
}
static buffer_t *get_free_buffer(void);
static buffer_t *page_LRU(void)
{
    list_node_t *node = list_remove_first(&fs_LRU_list);
    buffer_t *buf = list_node_parent(node, buffer_t, lru_node);
    fs_contend_buffer(buf); // 把数据写回磁盘
    buffer_t *ret = get_free_buffer();
    ASSERT(ret != NULL);
    return ret;
}
static buffer_t *get_free_buffer(void)
{
    if (list_count(&fs_buffer_free_list) == 0)
    {
        // 如果没有空闲buffer，页面替换算法
        // dbg_warning("Page cache exhausted, use page replacement algorithm here\r\n");
        return page_LRU();
    }
    list_node_t *node = list_remove_first(&fs_buffer_free_list);
    return list_node_parent(node, buffer_t, lnode);
}
void release_buffer(buffer_t *buf)
{
    memset(buf, 0, sizeof(buffer_t));
    list_insert_last(&fs_buffer_free_list, &buf->lnode);
}

/**
 * @brief 读取某磁盘或者扇区的几块到缓冲  注：一个缓冲存储8个连续扇区 4KB 即4个文件块
 * @param blk:这个块号指的是文件系统块号 2个扇区
 * @param modify:获取这个缓冲块的目的是啥？是要修改数据吗，还是仅仅读取
 */
buffer_t *fs_read_to_buffer(int major, int minor, int blk, bool modify)
{

    int ret;
    int orign_blk = blk;
    // 先检查这些块在不在缓冲
    blk = align_down(blk, 4);
    int arr[3] = {major, minor, blk};
    sys_mutex_lock(&fs_buf_mutex);

    rb_node_t *node = rb_tree_find_by(&fs_buffer_tree, arr, compare_key);
    buffer_t *target = NULL;
    if (node != fs_buffer_tree.nil)
    {
        // 在缓冲直接返回缓冲
        target = node_get_buffer(node);
        ASSERT(target->valid == true);
        list_remove(&fs_LRU_list, &target->lru_node);
        list_insert_last(&fs_LRU_list, &target->lru_node);
    }
    else
    {
        // 不在缓冲 分配缓冲并写入数据
        target = get_free_buffer();
        ASSERT(target != NULL);
        target->major = major;
        target->minor = minor;
        target->blk = blk;
        // 分配缓存页记录数据
        ph_addr_t phaddr = mm_bitmap_alloc_page();
        record_one_page(NULL, phaddr, phaddr, PAGE_TYPE_CHACHE);
        target->page = find_one_page(phaddr);
        target->dirty = false;
        target->valid = true;
        // 从磁盘上把这页数据写入缓存
        int devfd = MKDEV(major,minor);
        dev_open(devfd);
        int offset = blk * BLOCK_SECS;
        ret = dev_read(devfd,  phaddr, MEM_PAGE_SIZE, &offset);
        if (ret < 0)
        {
            dbg_error("disk blk write to buffer fail\r\n");
            remove_one_page(NULL, phaddr);
            release_buffer(target);
            sys_mutex_unlock(&fs_buf_mutex);
            return NULL;
        }
        dev_close(devfd);
        list_insert_last(&fs_LRU_list, &target->lru_node);
        rb_tree_insert(&fs_buffer_tree, target);
    }
    target->data = target->page->phaddr + (orign_blk - blk) * BLOCK_SIZE;
    // 如果modify位true,那么dirty肯定是true；如果modify位false，buffer.diry为其本身
    target->dirty = modify ? modify : target->dirty;
    sys_mutex_unlock(&fs_buf_mutex);
    return target;
}

/**
 * @brief 释放一个buffer_t（抢占一个buffer_t），如果数据脏，还要写回磁盘
 */
void fs_contend_buffer(buffer_t *buf)
{
    ASSERT(buf->valid == true);
    int ret;
    //sys_mutex_lock(&fs_buf_mutex);
    if (buf->dirty)
    {
        // 如果是脏数据，需要写回到磁盘
        int devfd =MKDEV(buf->major,buf->minor);
        dev_open(devfd);
        int offset = buf->blk * BLOCK_SECS;
        ret = dev_write(devfd,  buf->page->phaddr, MEM_PAGE_SIZE, &offset);
        if (ret < 0)
        {
            dbg_error("write back to disk fail\r\n");
            //sys_mutex_unlock(&fs_buf_mutex);
            return;
        }
        dev_close(devfd);
        buf->dirty = false;
    }
    rb_tree_remove(&fs_buffer_tree, buf);
    remove_one_page(NULL, buf->page->phaddr);
    release_buffer(buf);
    //sys_mutex_unlock(&fs_buf_mutex);
}
/**
 * @brief 同步某个buf
 */
void fs_sync_buffer(buffer_t *buf)
{
    int ret;
    sys_mutex_lock(&fs_buf_mutex);
    
    if (buf->dirty)
    {
        // 如果是脏数据，需要写回到磁盘
        int devfd =MKDEV(buf->major,buf->minor);
        dev_open(devfd);
        int offset = buf->blk * BLOCK_SECS;
        ret = dev_write(devfd,  buf->page->phaddr, MEM_PAGE_SIZE, &offset);
        if (ret < 0)
        {
            dbg_error("write back to disk fail\r\n");
            sys_mutex_unlock(&fs_buf_mutex);
            return;
        }
        dev_close(devfd);
        buf->dirty = false;
    }
    sys_mutex_unlock(&fs_buf_mutex);
}
/**
 * @brief 同步所有buffer
 */
void sys_sync(void)
{
    int ret;
    sys_mutex_lock(&fs_buf_mutex);
    list_node_t *node = list_first(&fs_LRU_list);
    while (node)
    {
        buffer_t *buf = list_node_parent(node, buffer_t, lru_node);
        sys_mutex_unlock(&fs_buf_mutex);
        fs_sync_buffer(buf);
        sys_mutex_lock(&fs_buf_mutex);
        node = node->next;
    }
    sys_mutex_unlock(&fs_buf_mutex);
}
