#include "dev/dev.h"
#include "cpu_instr.h"
#include "cpu_cfg.h"
#include "types.h"
#include "string.h"
#include "tools/rb_tree.h"
#include "tools/list.h"
#include "printk.h"
#include "mem/kmalloc.h"
#include "ipc/mutex.h"
#include "dev/dev.h"

// 设备树
static rb_tree_t dev_tree;
// 设备链表，方便遍历
static list_t dev_list;
static mutex_t mutex;

/**
 * 比较 `device_t` 的 `major``minor`
 */
static int device_compare(const void *a, const void *b)
{
    device_t *deva = (device_t *)a;
    device_t *devb = (device_t *)b;
    // 先比较 major 字段
    if (deva->desc.major != devb->desc.major)
    {
        return deva->desc.major - devb->desc.major;
    }
    // 如果 major 相同，比较 minor 字段
    return deva->desc.minor - devb->desc.minor;
}

/**
 * 获取 `device_t` 的 `rb_node_t`
 */
static rb_node_t *device_get_node(const void *data)
{
    return &((device_t *)data)->rbnode;
}

static device_t *node_get_device(rb_node_t *node)
{
    device_t *s = rb_node_parent(node, device_t, rbnode);
    return s;
}
static int compare_key(const void *key, const void *data)
{
    int *arr = (int *)key;
    int major = arr[0];
    int minor = arr[1];
    device_t *s = (device_t *)data; // **确保 data 是正确的 student_t**

    if (major != s->desc.major)
    {
        return major - s->desc.major;
    }
    return minor - s->desc.minor;
}
static void device_print(device_t *s)
{
    dbg_info("device name:%s,major=%d,minor=%d\r\n",
             s->desc.name,s->desc.major,s->desc.minor);
}

void devfs_init(void)
{
    sys_mutex_init(&mutex);
    rb_tree_init(&dev_tree, device_compare, device_get_node, node_get_device, NULL);
    list_init(&dev_list);
}

static device_t *dev_find(int major, int minor)
{
    sys_mutex_lock(&mutex);
    int arr[2] = {major, minor};
    rb_node_t *rbnode = rb_tree_find_by(&dev_tree, arr, compare_key);
    if (rbnode == dev_tree.nil)
    {
        dbg_error("dev major = %d,minor = %d,can`t find\r\n", major, minor);
        sys_mutex_unlock(&mutex);
        return NULL;
    }
    else
    {
        device_t* ret = node_get_device(rbnode);
        sys_mutex_unlock(&mutex);
        return ret;
    }
}

device_t *dev_get(int devfd)
{
    device_t *dev = (device_t *)devfd;
    return dev;
}

int dev_install(device_type_t type, int major, int minor, const char *name, void *data, dev_ops_t *ops)
{
    
    device_t *dev = (device_t *)kmalloc(sizeof(device_t));
    dev->devfd = (int)dev;
    dev->ops = ops;
    dev->desc.data = data;
    dev->desc.major = major;
    dev->desc.minor = minor;
    strcpy(dev->desc.name, name);
    dev->desc.open_count = 0;
    dev->desc.type = type;
    // 加入设备树
    sys_mutex_lock(&mutex);
    rb_tree_insert(&dev_tree, dev);
    list_insert_last(&dev_list, &dev->lnode);
    sys_mutex_unlock(&mutex);
    return 0;
}

void dev_show_all(void)
{
    rb_tree_inorder(&dev_tree,dev_tree.root,device_print);
}
int dev_open(int major, int minor, void *data)
{
    

    if (major >= DEV_MAJOR_MAX)
    {
        dbg_error("dev major = %d,minor_max = %d,out of boundry\r\n", major, DEV_MAJOR_MAX - 1);
        
        return -3;
    }
    device_t *dev = dev_find(major, minor);
    if (dev == NULL)
    {
        dbg_error("can`t find dev:major = %d,minor = %d\r\n", major, minor);
        
        return -4;
    }
    dev->desc.data = data;
    dev->desc.open_count++;
    if (dev->ops == NULL)
    {
        dbg_error("dev major = %d,minor = %d,don`t have ops\r\n", dev->desc.major, dev->desc.minor);
        
        return -1;
    }
   
    int ret = dev->ops->open(dev->devfd, 0);

    if (ret < 0)
    {

        return -2;
    }
    else
    {

        return dev->devfd;
    }
}
int dev_read(int devfd, int addr, char *buf, int size, int flag)
{

    device_t *dev = dev_get(devfd);
    if (!dev)
    {
        dbg_error("device not exisit\r\n");

        return -1;
    }

    return dev->ops->read(devfd, addr, buf, size, flag);
}
int dev_write(int devfd, int addr, char *buf, int size, int flag)
{

    device_t *dev = dev_get(devfd);
    if (!dev)
    {
        dbg_error("device not exisit\r\n");

        return -1;
    }

    return dev->ops->write(devfd, addr, buf, size, flag);
}
int dev_control(int devfd, int cmd, int arg0, int arg1)
{

    device_t *dev = dev_get(devfd);
    if (!dev)
    {
        dbg_error("device not exisit\r\n");

        return -1;
    }

    return dev->ops->control(devfd, cmd, arg0, arg1);
}
void dev_close(int devfd, int flag)
{
    device_t *dev = dev_get(devfd);
    if (!dev)
    {
        dbg_error("device not exisit\r\n");
        return -1;
    }
    return dev->ops->close(devfd, flag);
}