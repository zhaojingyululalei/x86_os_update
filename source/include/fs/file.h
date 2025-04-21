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
    int ref;
    
}file_t;


file_t * file_alloc (void);
void file_free (file_t * file);
void file_inc_ref (file_t * file);
void file_table_init (void);

#endif