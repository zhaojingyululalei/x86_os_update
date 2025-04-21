#ifndef __DIRENT_H
#define __DIRENT_H

#define DIR_MAX_NAME    128
typedef struct _DIR_t
{

}DIR;
/**
 * @brief 全局抽象目录形式
 */
typedef struct dirent{
    int nr;
    char name[DIR_MAX_NAME];
};

#endif