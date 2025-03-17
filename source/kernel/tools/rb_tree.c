#include "tools/rb_tree.h"
#include "string.h"
static  rb_node_t nil;
// 初始化红黑树
void rb_tree_init(rb_tree_t *tree,int (*compare)(const void*, const void*) , rb_node_t* (*get_node)(const void*), void* (*get_parent)(rb_node_t*)) {
    if (!tree) return;

    // 创建哨兵节点
    tree->nil = &nil;
    tree->nil->color = BLACK; // 哨兵节点为黑色
    tree->nil->left = tree->nil->right = tree->nil->parent = tree->nil; // 指向自己

    // 初始化根节点
    tree->root = tree->nil; // 初始时根节点指向哨兵节点
    tree->count = 0;
    tree->compare = (int (*)(const void*, const void*))compare;
    tree->get_node = (rb_node_t* (*)(const void*))get_node;
    tree->get_parent = (void* (*)(rb_node_t*))get_parent;
}

// 左旋
void left_rotate(rb_tree_t *tree, rb_node_t *x)
{
    if (!tree || !x || x == tree->nil)
        return; // 检查指针是否为空

    rb_node_t *y = x->right;
    if (!y || y == tree->nil)
        return; // 检查 y 是否为空

    x->right = y->left;
    if (y->left != tree->nil)
    {
        y->left->parent = x;
    }

    y->parent = x->parent;
    if (x->parent == tree->nil)
    {
        tree->root = y;
    }
    else if (x == x->parent->left)
    {
        x->parent->left = y;
    }
    else
    {
        x->parent->right = y;
    }

    y->left = x;
    x->parent = y;
}

// 右旋
void right_rotate(rb_tree_t *tree, rb_node_t *y)
{
    if (!tree || !y || y == tree->nil)
        return; // 检查指针是否为空

    rb_node_t *x = y->left;
    if (!x || x == tree->nil)
        return; // 检查 x 是否为空

    y->left = x->right;
    if (x->right != tree->nil)
    {
        x->right->parent = y;
    }

    x->parent = y->parent;
    if (y->parent == tree->nil)
    {
        tree->root = x;
    }
    else if (y == y->parent->right)
    {
        y->parent->right = x;
    }
    else
    {
        y->parent->left = x;
    }

    x->right = y;
    y->parent = x;
}

// 插入修复
void rb_insert_fixup(rb_tree_t *tree, rb_node_t *z)
{
    if (!tree || !z || z == tree->nil)
        return; // 检查指针是否为空

    // 修复红黑树性质
    while (z->parent != tree->nil && z->parent->color == RED)
    {
        if (z->parent == z->parent->parent->left)
        {
            rb_node_t *y = z->parent->parent->right; // 叔节点
            if (y->color == RED)
            {
                // 情况 1：叔节点为红色
                z->parent->color = BLACK;
                y->color = BLACK;
                z->parent->parent->color = RED;
                z = z->parent->parent; // 向上迭代
            }
            else
            {
                if (z == z->parent->right)
                {
                    // 情况 2：z 是右孩子
                    z = z->parent;
                    left_rotate(tree, z);
                }
                // 情况 3：z 是左孩子
                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                right_rotate(tree, z->parent->parent);
            }
        }
        else
        {
            // 对称的情况
            rb_node_t *y = z->parent->parent->left; // 叔节点
            if (y->color == RED)
            {
                // 情况 1：叔节点为红色
                z->parent->color = BLACK;
                y->color = BLACK;
                z->parent->parent->color = RED;
                z = z->parent->parent; // 向上迭代
            }
            else
            {
                if (z == z->parent->left)
                {
                    // 情况 2：z 是左孩子
                    z = z->parent;
                    right_rotate(tree, z);
                }
                // 情况 3：z 是右孩子
                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                left_rotate(tree, z->parent->parent);
            }
        }
    }
    // 确保根节点为黑色
    tree->root->color = BLACK;
}

// 插入节点
int rb_tree_insert(rb_tree_t *tree, void *data)
{
    if (!tree || !data)
        return -1; // 检查指针是否为空

    rb_node_t *z = tree->get_node(data);
    if (!z)
        return -1; // 检查获取的节点是否为空

    rb_node_t *y = tree->nil;
    rb_node_t *x = tree->root;

    while (x != tree->nil)
    {
        y = x;
        if (tree->compare(data, tree->get_parent(x)) < 0)
        {
            x = x->left;
        }
        else
        {
            x = x->right;
        }
    }

    z->parent = y;
    if (y == tree->nil)
    {
        tree->root = z;
    }
    else if (tree->compare(data, tree->get_parent(y)) < 0)
    {
        y->left = z;
    }
    else
    {
        y->right = z;
    }

    z->left = tree->nil;
    z->right = tree->nil;
    z->color = RED;
    rb_insert_fixup(tree, z);
    tree->count++;
    return 0;
}

// 删除修复
void rb_delete_fixup(rb_tree_t *tree, rb_node_t *x)
{
    if (!tree || !x || x == tree->nil)
        return; // 检查指针是否为空

    while (x != tree->root && x->color == BLACK)
    {
        if (x == x->parent->left)
        {
            rb_node_t *w = x->parent->right;
            if (w->color == RED)
            {
                w->color = BLACK;
                x->parent->color = RED;
                left_rotate(tree, x->parent);
                w = x->parent->right;
            }
            if (w->left->color == BLACK && w->right->color == BLACK)
            {
                w->color = RED;
                x = x->parent;
            }
            else
            {
                if (w->right->color == BLACK)
                {
                    w->left->color = BLACK;
                    w->color = RED;
                    right_rotate(tree, w);
                    w = x->parent->right;
                }
                w->color = x->parent->color;
                x->parent->color = BLACK;
                w->right->color = BLACK;
                left_rotate(tree, x->parent);
                x = tree->root;
            }
        }
        else
        {
            rb_node_t *w = x->parent->left;
            if (w->color == RED)
            {
                w->color = BLACK;
                x->parent->color = RED;
                right_rotate(tree, x->parent);
                w = x->parent->left;
            }
            if (w->right->color == BLACK && w->left->color == BLACK)
            {
                w->color = RED;
                x = x->parent;
            }
            else
            {
                if (w->left->color == BLACK)
                {
                    w->right->color = BLACK;
                    w->color = RED;
                    left_rotate(tree, w);
                    w = x->parent->left;
                }
                w->color = x->parent->color;
                x->parent->color = BLACK;
                w->left->color = BLACK;
                right_rotate(tree, x->parent);
                x = tree->root;
            }
        }
    }
    x->color = BLACK;
}

// 删除节点
int rb_tree_remove(rb_tree_t *tree, void *data)
{
    if (!tree || !data)
        return -1; // 检查指针是否为空

    rb_node_t *z = tree->get_node(data);
    if (z == tree->nil)
        return -1; // 未找到节点

    rb_node_t *y = z;
    rb_node_t *x;
    rb_color_t y_original_color = y->color;

    if (z->left == tree->nil)
    {
        x = z->right;
        if (z->parent == tree->nil)
        {
            tree->root = x;
        }
        else if (z == z->parent->left)
        {
            z->parent->left = x;
        }
        else
        {
            z->parent->right = x;
        }
        x->parent = z->parent;
    }
    else if (z->right == tree->nil)
    {
        x = z->left;
        if (z->parent == tree->nil)
        {
            tree->root = x;
        }
        else if (z == z->parent->left)
        {
            z->parent->left = x;
        }
        else
        {
            z->parent->right = x;
        }
        x->parent = z->parent;
    }
    else
    {
        y = z->right;
        while (y->left != tree->nil)
        {
            y = y->left;
        }
        y_original_color = y->color;
        x = y->right;
        if (y->parent == z)
        {
            x->parent = y;
        }
        else
        {
            if (y->parent == tree->nil)
            {
                tree->root = x;
            }
            else if (y == y->parent->left)
            {
                y->parent->left = x;
            }
            else
            {
                y->parent->right = x;
            }
            x->parent = y->parent;
            y->right = z->right;
            y->right->parent = y;
        }
        if (z->parent == tree->nil)
        {
            tree->root = y;
        }
        else if (z == z->parent->left)
        {
            z->parent->left = y;
        }
        else
        {
            z->parent->right = y;
        }
        y->parent = z->parent;
        y->left = z->left;
        y->left->parent = y;
        y->color = z->color;
    }

    if (y_original_color == BLACK)
    {
        rb_delete_fixup(tree, x);
    }

    tree->count--;
    return 0;
}

// 查找节点
rb_node_t *rb_tree_find(rb_tree_t *tree, const void *data)
{
    if (!tree || !data)
        return tree->nil; // 检查指针是否为空

    rb_node_t *x = tree->root;
    while (x != tree->nil)
    {
        int cmp = tree->compare(data, tree->get_parent(x));
        if (cmp == 0)
        {
            return x;
        }
        else if (cmp < 0)
        {
            x = x->left;
        }
        else
        {
            x = x->right;
        }
    }
    return tree->nil;
}

// 中序遍历
void rb_tree_inorder(rb_tree_t *tree, rb_node_t *node, void (*print)(const void *))
{
    if (!tree || !node || node == tree->nil || !print)
        return; // 检查指针是否为空

    rb_tree_inorder(tree, node->left, print);
    print(tree->get_parent(node));
    rb_tree_inorder(tree, node->right, print);
}

void rb_tree_clear(rb_tree_t *tree)
{
    tree->count = 0;
    tree->root = tree->nil = NULL;
}

// 根据键查找节点
rb_node_t *rb_tree_find_by(rb_tree_t *tree, const void *key, int (*compare_key)(const void *, const void *))
{
    if (!tree || !key || !compare_key)
        return tree->nil; // 检查指针是否为空

    rb_node_t *x = tree->root;
    while (x != tree->nil)
    {
        int cmp = compare_key(key, tree->get_parent(x));
        if (cmp == 0)
        {
            return x;
        }
        else if (cmp < 0)
        {
            x = x->left;
        }
        else
        {
            x = x->right;
        }
    }
    return tree->nil;
}

// 查找第一个大于或等于指定 key 的节点
rb_node_t *rb_tree_find_first_greater_or_equal(rb_tree_t *tree, const void *key, int (*compare_key)(const void *, const void *)) {
    if (!tree || !key || !compare_key)
        return tree->nil; // 检查指针是否为空

    rb_node_t *x = tree->root;
    rb_node_t *result = tree->nil;

    while (x != tree->nil) {
        int cmp = compare_key(key, tree->get_parent(x));
        if (cmp <= 0) {
            result = x;
            x = x->left;
        } else {
            x = x->right;
        }
    }

    return result;
}

// 查找第一个小于或等于指定 key 的节点
rb_node_t *rb_tree_find_first_less_or_equal(rb_tree_t *tree, const void *key, int (*compare_key)(const void *, const void *)) {
    if (!tree || !key || !compare_key)
        return tree->nil; // 检查指针是否为空

    rb_node_t *x = tree->root;
    rb_node_t *result = tree->nil;

    while (x != tree->nil) {
        int cmp = compare_key(key, tree->get_parent(x));
        if (cmp >= 0) {
            result = x;
            x = x->right;
        } else {
            x = x->left;
        }
    }

    return result;
}

// 查找红黑树中的最小节点
static rb_node_t *rb_tree_minimum(rb_tree_t *tree, rb_node_t *node) {
    if (!tree || !node || node == tree->nil)
        return tree->nil; // 检查指针是否为空

    // 一直向左遍历，直到找到最左边的节点
    while (node->left != tree->nil) {
        node = node->left;
    }
    return node;
}

// 查找整棵树的最小节点
rb_node_t *rb_tree_find_min(rb_tree_t *tree) {
    if (!tree || tree->root == tree->nil)
        return tree->nil; // 检查树是否为空

    return rb_tree_minimum(tree, tree->root);
}

// 查找红黑树中的最大节点
static rb_node_t *rb_tree_maximum(rb_tree_t *tree, rb_node_t *node) {
    if (!tree || !node || node == tree->nil)
        return tree->nil; // 检查指针是否为空

    // 一直向右遍历，直到找到最右边的节点
    while (node->right != tree->nil) {
        node = node->right;
    }
    return node;
}

// 查找整棵树的最大节点
rb_node_t *rb_tree_find_max(rb_tree_t *tree) {
    if (!tree || tree->root == tree->nil)
        return tree->nil; // 检查树是否为空

    return rb_tree_maximum(tree, tree->root);
}