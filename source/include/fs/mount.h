#ifndef __MOUNT_H
#define __MOUNT_H

#include "types.h"
#include "fs/fs.h"
#include "tools/list.h"
#define MAX_PATH_BUF 256

typedef struct _mount_point_t {
    char point_path[MAX_PATH_BUF];
    int major;
    int minor;
    fs_type_t type;
    list_node_t node;

    int father_major;
    int father_minor;
    fs_type_t father_type;
}mount_point_t;
mount_point_t* find_point_by_path(const char* abs_path);

#endif