#include "tools/gbitmap.h"

/**
 * @brief 设置位图 某个位为value，value为 0或者1
 */
void gbitmap_set(char* bitmap, int idx, int value) {
    int byte_idx = idx / 8;  // 计算字节索引
    int bit_idx = idx % 8;   // 计算位索引
    if (value) {
        bitmap[byte_idx] |= (1 << bit_idx);  // 设置位为1
    } else {
        bitmap[byte_idx] &= ~(1 << bit_idx); // 设置位为0
    }
}

/**
 * @brief 清除某一位为0
 */
void gbitmap_clear(char* bitmap, int idx) {
    gbitmap_set(bitmap, idx, 0);
}

/**
 * @brief 设置某一位为1
 */
void gbitmap_set_bit(char* bitmap, int idx) {
    gbitmap_set(bitmap, idx, 1);
}

/**
 * @brief 读取某一位的值，并返回
 */
int gbitmap_get(char* bitmap, int idx) {
    int byte_idx = idx / 8;  // 计算字节索引
    int bit_idx = idx % 8;   // 计算位索引
    return (bitmap[byte_idx] >> bit_idx) & 1;  // 返回位的值
}

/**
 * @brief 判断某一位是否为0(是否空闲)
 */
int gbitmap_is_free(char* bitmap, int idx) {
    return gbitmap_get(bitmap, idx) == 0;
}

/**
 * @brief 判断某一位是否为1(是否占用)
 */
int gbitmap_is_used(char* bitmap, int idx) {
    return gbitmap_get(bitmap, idx) == 1;
}

/**
 * @brief 获取第一个空闲位
 */
int gbitmap_find_first_free(char* bitmap, int size) {
    for (int i = 0; i < size; i++) {
        if (gbitmap_is_free(bitmap, i)) {
            return i;  // 返回第一个空闲位的索引
        }
    }
    return -1;  // 没有找到空闲位
}