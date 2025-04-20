#ifndef __FILE_H
#define __FILE_H
#include "fs/devfs/devfs.h"
#include "fs/inode.h"
typedef struct _file_t
{
    union 
    {
        dev_t dev;
        inode_t* inode;
        int sockfd;
    };
    int pos;
    
}file_t;


#endif