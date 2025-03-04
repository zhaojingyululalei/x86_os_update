#include "algrithm.h"
#include "types.h"

/**
 * @brief 向下对齐到边界 
 * @param boundry 必须是2的次方 1 2 4 8....
 */
uint32_t align_down(uint32_t addr,uint32_t boundry) {
    return addr & ~(boundry - 1);
}

/**
 * @brief 向上对齐到边界
 * @param boundry 必须是2的次方 1 2 4 8....
 */
uint32_t align_up(uint32_t addr,uint32_t boundry) {
    return (addr + boundry - 1) & ~(boundry - 1);
}