#include "fs/inode.h"

rb_tree_t inode_tree;
static int inode_compare(void *a, void *b)
{
    inode_t* ia = (inode_t*)a;
    inode_t* ib = (inode_t*)b;
    
    if(ia->dev!= ib->dev)
    {
        return ia->dev - ib->dev;
    }

    if(ia->nr!=ib->nr)
    {
        return ia->nr-  ib->nr;
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
    if(dev != ib->dev)
    {
        return dev - ib->dev;
    }
    if(nr != ib->nr)
    {
        return nr - ib->nr;
    }
    return 0;
}

static inode_opts_t* fs_inode_opts[0xFF];

void register_inode_operations(inode_opts_t* opts,fs_type_t type)
{
    fs_inode_opts[type] = opts;
    
}
void inode_tree_init(void)
{
    rb_tree_init(&inode_tree, inode_compare, inode_get_rbnode, rbnode_get_inode, NULL);
}
rb_tree_t* get_inode_tree(void)
{
    return &inode_tree;
}