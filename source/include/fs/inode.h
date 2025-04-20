#ifndef __INODE_H
#define __INODE_H
#include "fs/fs.h"
typedef struct _inode_t
{
    fs_type_t type;
    void* fsinode;
    
}inode_t;


#endif