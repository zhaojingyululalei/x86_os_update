#include "fs/inode.h"
#include "dev/dev.h"
#include "dev/ide.h"
#include "fs/inode.h"
#include "inode.h"
rb_tree_t inode_tree;
static int inode_compare(void *a, void *b)
{
    inode_t *ia = (inode_t *)a;
    inode_t *ib = (inode_t *)b;

    if (ia->dev != ib->dev)
    {
        return ia->dev - ib->dev;
    }

    if (ia->nr != ib->nr)
    {
        return ia->nr - ib->nr;
    }
    return 0;
}
static rb_node_t *inode_get_rbnode(const void *data)
{
    inode_t *inode = data;
    return &inode->rbnode;
}
static inode_t *rbnode_get_inode(rb_node_t *node)
{
    return rb_node_parent(node, inode_t, rbnode);
}
/**
 * @brief 通过devid和nr来查找inode的比较函数
 */
int inode_compare_key(const void *key, const void *data)
{
    int *arr = (int *)key;
    inode_t *ib = (inode_t *)data;
    dev_t dev = arr[0];
    int nr = arr[1];
    if (dev != ib->dev)
    {
        return dev - ib->dev;
    }
    if (nr != ib->nr)
    {
        return nr - ib->nr;
    }
    return 0;
}


static inode_opts_t *fs_inode_opts[0xFF];
void register_inode_operations(inode_opts_t *opts, fs_type_t type)
{
    fs_inode_opts[type] = opts;
}
void inode_tree_init(void)
{
    rb_tree_init(&inode_tree, inode_compare, inode_get_rbnode, rbnode_get_inode, NULL);
}
rb_tree_t *get_inode_tree(void)
{
    return &inode_tree;
}



inode_t* inode_get(dev_t dev,int nr)
{
    fs_type_t type = dev_control(dev, PART_CMD_FS_TYPE, 0, 0);
    return fs_inode_opts[type]->get(dev,nr);
}
inode_t* inode_create(dev_t dev, int nr) {
    fs_type_t type = dev_control(dev, PART_CMD_FS_TYPE, 0, 0);
    return fs_inode_opts[type]->create(dev, nr);
}

void inode_destroy(inode_t *inode) {
    if (inode == NULL) return;
    fs_type_t type = dev_control(inode->dev, PART_CMD_FS_TYPE, 0, 0);
    fs_inode_opts[type]->destroy(inode);
}

int inode_free(dev_t dev, int nr) {
    fs_type_t type = dev_control(dev, PART_CMD_FS_TYPE, 0, 0);
    return fs_inode_opts[type]->free(dev, nr);
}

int inode_alloc(dev_t dev) {
    fs_type_t type = dev_control(dev, PART_CMD_FS_TYPE, 0, 0);
    return fs_inode_opts[type]->alloc(dev);
}

int inode_read(inode_t *inode, char *buf, int buf_size, int whence, int read_size) {
    if (inode == NULL) return -1;
    fs_type_t type = dev_control(inode->dev, PART_CMD_FS_TYPE, 0, 0);
    return fs_inode_opts[type]->read(inode, buf, buf_size, whence, read_size);
}

int inode_write(inode_t *inode, const char *buf, int buf_size, int whence, int write_size) {
    if (inode == NULL) return -1;
    fs_type_t type = dev_control(inode->dev, PART_CMD_FS_TYPE, 0, 0);
    return fs_inode_opts[type]->write(inode, buf, buf_size, whence, write_size);
}

int inode_truncate(inode_t *inode, uint32_t new_size) {
    if (inode == NULL) return -1;
    fs_type_t type = dev_control(inode->dev, PART_CMD_FS_TYPE, 0, 0);
    return fs_inode_opts[type]->truncate(inode, new_size);
}

int inode_getinfo(inode_t *inode, dev_t dev, int nr) {
    if (inode == NULL) return -1;
    fs_type_t type = dev_control(dev, PART_CMD_FS_TYPE, 0, 0);
    return fs_inode_opts[type]->getinfo(inode, dev, nr);
}

int inode_find_entry(inode_t *inode, const char *name, struct dirent *entry) {
    if (inode == NULL) return -1;
    fs_type_t type = dev_control(inode->dev, PART_CMD_FS_TYPE, 0, 0);
    return fs_inode_opts[type]->find_entry(inode, name, entry);
}

int inode_add_entry(inode_t *inode, struct dirent *entry) {
    if (inode == NULL) return -1;
    fs_type_t type = dev_control(inode->dev, PART_CMD_FS_TYPE, 0, 0);
    return fs_inode_opts[type]->add_entry(inode, entry);
}

int inode_delete_entry(inode_t *inode, struct dirent *entry) {
    if (inode == NULL) return -1;
    fs_type_t type = dev_control(inode->dev, PART_CMD_FS_TYPE, 0, 0);
    return fs_inode_opts[type]->delete_entry(inode, entry);
}
inode_t *get_dev_root_inode(dev_t dev)
{
    fs_type_t type = dev_control(dev, PART_CMD_FS_TYPE, 0, 0);
    return fs_inode_opts[type]->get_dev_root_inode(dev);
}
int inode_mkdir(inode_t *inode, const char *dir_name, mode_t mode)
{

    fs_type_t type = dev_control(inode->dev, PART_CMD_FS_TYPE, 0, 0);
    return fs_inode_opts[type]->mkdir(inode,dir_name,mode);
}

inode_t *inode_open(inode_t *inode, const char *file_name, uint32_t flag, mode_t mode)
{
    fs_type_t type = dev_control(inode->dev, PART_CMD_FS_TYPE, 0, 0);
    return fs_inode_opts[type]->open(inode,file_name,flag,mode);
}

void inode_close(inode_t *inode)
{
    fs_type_t type = dev_control(inode->dev, PART_CMD_FS_TYPE, 0, 0);
    return fs_inode_opts[type]->close(inode);
}

void inode_fsync(inode_t *inode)
{
    fs_type_t type = dev_control(inode->dev, PART_CMD_FS_TYPE, 0, 0);
    return fs_inode_opts[type]->fsync(inode);
}

void inode_stat(inode_t *inode, file_stat_t *state)
{
    fs_type_t type = dev_control(inode->dev, PART_CMD_FS_TYPE, 0, 0);
    return fs_inode_opts[type]->stat(inode,state);
}

int inode_rmdir(inode_t *inode, const char *name)
{
    fs_type_t type = dev_control(inode->dev, PART_CMD_FS_TYPE, 0, 0);
    return fs_inode_opts[type]->rmdir(inode,name);
}

int inode_root_nr(fs_type_t type)
{
    return fs_inode_opts[type]->root_nr();
}

uint16_t inode_acquire_uid(inode_t *inode)
{
    fs_type_t type = dev_control(inode->dev, PART_CMD_FS_TYPE, 0, 0);
    return fs_inode_opts[type]->acquire_uid(inode);
}
uint32_t inode_acquire_size(inode_t *inode)
{
    fs_type_t type = dev_control(inode->dev, PART_CMD_FS_TYPE, 0, 0);
    return fs_inode_opts[type]->acquire_size(inode);
}
uint32_t inode_acquire_mtime(inode_t *inode)
{
    fs_type_t type = dev_control(inode->dev, PART_CMD_FS_TYPE, 0, 0);
    return fs_inode_opts[type]->acquire_mtime(inode);
}
uint32_t inode_acquire_atime(inode_t *inode)
{
    fs_type_t type = dev_control(inode->dev, PART_CMD_FS_TYPE, 0, 0);
    return fs_inode_opts[type]->acquire_atime(inode);
}
uint32_t inode_acquire_ctime(inode_t *inode)
{
    fs_type_t type = dev_control(inode->dev, PART_CMD_FS_TYPE, 0, 0);
    return fs_inode_opts[type]->acquire_ctime(inode);
}
uint8_t inode_acquire_gid(inode_t *inode)
{
    fs_type_t type = dev_control(inode->dev, PART_CMD_FS_TYPE, 0, 0);
    return fs_inode_opts[type]->acquire_gid(inode);
}
uint8_t inode_acquire_nlinks(inode_t *inode)
{
    fs_type_t type = dev_control(inode->dev, PART_CMD_FS_TYPE, 0, 0);
    return fs_inode_opts[type]->acquire_nlinks(inode);
}

void inode_modify_nlinks(inode_t *inode, int n)
{
    fs_type_t type = dev_control(inode->dev, PART_CMD_FS_TYPE, 0, 0);
    return fs_inode_opts[type]->modify_nlinks(inode,n);
}
