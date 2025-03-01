#include "stdarg.h"

// 定义一个可变参数函数
static void print_numbers(int count, ...) {
    va_list args;  // char* args;
    va_start(args, count);  // args指向count的下一个参数

    for (int i = 0; i < count; i++) {
        int value = va_arg(args, int);  // 获取该参数的值，并且向下一个参数移动
    }

    va_end(args);  // args = null
    int x = 0;
    return;
}
void stdarg_test(void){
    char a = 'a';
    short b = 16;
    int c = 65535;
    print_numbers(3,6,7,8);
}