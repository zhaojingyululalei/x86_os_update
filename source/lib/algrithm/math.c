

unsigned int power(unsigned int base, unsigned int exponent) {
    unsigned int result = 1;
    
    // 指数为 0 时，任何数的 0 次方都是 1
    if (exponent == 0) {
        return 1;
    }

    // 逐步相乘，计算 base^exponent
    while (exponent > 0) {
        result *= base;
        exponent--;
    }

    return result;
}
