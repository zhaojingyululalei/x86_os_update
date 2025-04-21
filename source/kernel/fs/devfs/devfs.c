#include "fs/devfs/devfs.h"
#include "fs/fs.h"
#include "dev/dev.h"
int devfs_open(file_t *file, const char *path, int flags, int mode)
{
    dev_t dev = file->dev;
    int major = MAJOR(dev);
    int minor = MINOR(dev);
    return dev_open(dev);

}

int devfs_read(file_t *file, char *buf, int size)
{
    dev_t dev = file->dev;
    return dev_read(dev,buf,size,&file->pos);
}

int devfs_write(file_t *file, char *buf, int size)
{
    // 函数的实现
    dev_t dev = file->dev;
    return dev_write(dev,buf,size,&file->pos);
}
int devfs_lseek(file_t* file,int offset,int whence)
{
    dev_t dev = file->dev;
    return dev_lseek(dev,&file->pos,offset,whence);

}
void devfs_close(file_t *file)
{
    dev_t dev = file->dev;
    return dev_close(dev);
    // 函数的实现
}



int devfs_stat(file_t *file, file_stat_t *st)
{
    dev_t dev = file->dev;
    
    // 函数的实现
}

int devfs_ioctl(file_t *file, int cmd, int arg0, int arg1)
{
    dev_t dev = file->dev;
    return dev_control(dev,cmd,arg0,arg1);
    // 函数的实现
}
fs_op_t devfs_opts = {
    .open = devfs_open,
    .read = devfs_read,
    .write = devfs_write,
    .close = devfs_close,
    .lseek = devfs_lseek,
    .stat = devfs_stat,
    .ioctl = devfs_ioctl

};