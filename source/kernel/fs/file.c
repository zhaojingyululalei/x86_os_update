

#include "fs/file.h"
#include "ipc/mutex.h"
#include "string.h"
#define FILE_TABLE_SIZE 1024
static file_t file_table[FILE_TABLE_SIZE];      // 系统中可打开的文件表
static mutex_t file_alloc_mutex;                // 访问file_table的互斥信号量

/**
 * @brief 分配一个文件描述符
 */
file_t * file_alloc (void) {
    file_t * file = (file_t *)0;

    sys_mutex_lock(&file_alloc_mutex);
    for (int i = 0; i < FILE_TABLE_SIZE; i++) {
        file_t * p_file = file_table + i;
        if (p_file->ref == 0) {
			memset(p_file, 0, sizeof(file_t));
            p_file->ref = 1;
			file = p_file;
            break;
        }
    }
    sys_mutex_unlock(&file_alloc_mutex);
    return file;
}

/**
 * @brief 释放文件描述符
 */
void file_free (file_t * file) {
    sys_mutex_lock(&file_alloc_mutex);
    if (file->ref) {
        file->ref--;
        
    }
    sys_mutex_unlock(&file_alloc_mutex);
}

/**
 * @brief 增加file的引用计数
 */
void file_inc_ref (file_t * file) {
    sys_mutex_lock(&file_alloc_mutex);
	file->ref++;
    sys_mutex_unlock(&file_alloc_mutex);
}

/**
 * @brief 文件表初始化
 */
void file_table_init (void) {
	// 文件描述符表初始化
	memset(file_table, 0, sizeof(file_table));
	sys_mutex_init(&file_alloc_mutex);
}

