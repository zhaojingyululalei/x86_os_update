#ifndef __MINIX_FS_H
#define __MINIX_FS_H
#include "types.h"
#include "time/time.h"
#include "cpu_cfg.h"

#include "fs/minix_cfg.h"







int minix_mkdir (char *path, int mode);
int minix_rmdir(const char* path);
int minix_link(const char *old_path, const char *new_path);
int minix_unlink(const char *path);
#endif