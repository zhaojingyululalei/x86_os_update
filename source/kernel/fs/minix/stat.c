
#include "fs/minix_fs.h"
#include "fs/stat.h"
#include "printk.h"
int print_mode(uint16_t mode) {
    char str[11] = {0};
    
    // 文件类型
    if (ISREG(mode))  str[0] = '-';
    else if (ISDIR(mode)) str[0] = 'd';
    else if (ISCHR(mode)) str[0] = 'c';
    else if (ISBLK(mode)) str[0] = 'b';
    else if (ISFIFO(mode)) str[0] = 'p';
    else if (ISLNK(mode)) str[0] = 'l';
    else if (ISSOCK(mode)) str[0] = 's';
    else str[0] = '?';

    // 用户权限
    str[1] = (mode & IRUSR) ? 'r' : '-';
    str[2] = (mode & IWUSR) ? 'w' : '-';
    str[3] = (mode & IXUSR) ? 'x' : '-';

    // 组权限
    str[4] = (mode & IRGRP) ? 'r' : '-';
    str[5] = (mode & IWGRP) ? 'w' : '-';
    str[6] = (mode & IXGRP) ? 'x' : '-';

    // 其他权限
    str[7] = (mode & IROTH) ? 'r' : '-';
    str[8] = (mode & IWOTH) ? 'w' : '-';
    str[9] = (mode & IXOTH) ? 'x' : '-';

    // 特殊权限位
    if (mode & ISUID) str[3] = (str[3] == 'x') ? 's' : 'S';
    if (mode & ISGID) str[6] = (str[6] == 'x') ? 's' : 'S';
    if (mode & ISVTX) str[9] = (str[9] == 'x') ? 't' : 'T';

    dbg_info("%s\r\n", str);
    return 0;
}