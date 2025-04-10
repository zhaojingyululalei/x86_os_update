#include "fs/fs.h"
#include "fs/minix_fs.h"
/**
 *  const char *path = "dir/file.txt";
    const char *entry = "dir";
    char *next;
    bool result = match_name(path, entry, &next, 3); 
    结果：返回true，并且next指向“file.txt”这个字符串
 */
// 判断文件名是否相等
bool match_name(const char *name, const char *entry_name, char **next, int count)
{
    char *lhs = (char *)name;
    char *rhs = (char *)entry_name;

    while (*lhs == *rhs && *lhs != EOS && *rhs != EOS && count--)
    {
        lhs++;
        rhs++;
    }

    if (*rhs && count)
        return false;
    if (*lhs && !IS_SEPARATOR(*lhs))
        return false;
    if (IS_SEPARATOR(*lhs))
        lhs++;
    *next = lhs;
    return true;
}

minix_dentry_t* find_entry(inode_t* inode,const char* name,char **next)
{
    
}