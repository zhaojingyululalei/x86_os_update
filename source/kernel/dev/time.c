#include "time/time.h"
#include "cpu_instr.h"
#include "irq/irq.h"
#include "string.h"
#define CMOS_ADDRESS 0x70
#define CMOS_DATA 0x71

/**
 * @brief   从 CMOS 读取指定寄存器的值
 */
static inline uint8_t read_cmos(uint8_t reg)
{
    outb(CMOS_ADDRESS, reg); // 设置寄存器地址
    return inb(CMOS_DATA);   // 从 CMOS 数据端口读取数据
}


time_t startup_time;

// 时间单位定义
#define MINUTE 60
#define HOUR (60 * MINUTE)
#define DAY (24 * HOUR)
#define YEAR (365 * DAY)
#define LEAP_YEAR_DAY (366 * DAY)

// 每月天数定义（默认 2 月为 28 天）
static const int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// 判断是否为闰年
static int is_leap_year(int year)
{
    return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

/**
 * @brief 将本地时间转换为时间戳,总秒数
 */
time_t mktime(const tm_t *tm)
{
    int year = tm->tm_year;
    long days = 0;

    // 处理 Y2K 问题
    if (year < 70)
        year += 100; // 把 00-69 解析为 2000-2069
    year += 1900;    // 得到实际年份

    // 累加从 1970 年到指定年份的天数
    for (int y = 1970; y < year; y++)
    {
        days += is_leap_year(y) ? 366 : 365;
    }

    // 累加当年月份的天数
    for (int m = 0; m < tm->tm_mon; m++)
    {
        days += days_in_month[m];
        if (m == 1 && is_leap_year(year))
        { // 2 月在闰年多 1 天
            days += 1;
        }
    }

    // 累加当月天数
    days += tm->tm_mday - 1;

    // 计算总秒数
    return days * DAY + tm->tm_hour * HOUR + tm->tm_min * MINUTE + tm->tm_sec;
}

/**
 * @brief 计算时间戳对应的本地时间
 */
int localtime(tm_t *tm, const time_t time)
{
    if (tm == NULL)
        return -1;

    int year = 1970;
    int days = time / DAY;
    int remaining_secs = time % DAY;

    // 解析小时、分钟、秒
    tm->tm_hour = remaining_secs / HOUR;
    remaining_secs %= HOUR;
    tm->tm_min = remaining_secs / MINUTE;
    tm->tm_sec = remaining_secs % MINUTE;

    // 计算年份
    while (days >= (is_leap_year(year) ? 366 : 365))
    {
        days -= is_leap_year(year) ? 366 : 365;
        year++;
    }
    tm->tm_year = year - 1900; // 转换为 1900 年的偏移
    tm->tm_year = tm->tm_year >= 100 ? tm->tm_year - 100 : tm->tm_year;
    // 计算一年中的第几天
    tm->tm_yday = days;

    // 计算月份和日期
    int month = 0;
    while (days >= days_in_month[month] + (month == 1 && is_leap_year(year) ? 1 : 0))
    {
        days -= days_in_month[month] + (month == 1 && is_leap_year(year) ? 1 : 0);
        month++;
    }
    tm->tm_mon = month;
    tm->tm_mday = days + 1;

    // 计算星期几（1970-01-01 是星期四）
    tm->tm_wday = (time / DAY + 4) % 7;

    // 假设不使用夏令时
    tm->tm_isdst = 0;

    return 0;
}
/**
 * @brief 格式化时间为字符串
 *      %Y：4 位数的年份（如 2025）。

        %y：2 位数的年份（如 25）。

        %m：2 位数的月份（01-12）。

        %d：2 位数的日期（01-31）。

        %H：2 位数的小时（00-23）。

        %M：2 位数的分钟（00-59）。

        %S：2 位数的秒数（00-59）。
 */

int strtime(char* buf, int buf_size, const char* format, const tm_t* time) {
    if (buf == NULL || format == NULL || time == NULL || buf_size <= 0) {
        return -1;  // 参数检查
    }

    int written = 0;  // 已写入的字符数
    const char* p = format;

    while (*p != '\0' && written < buf_size - 1) {
        if (*p == '%') {
            p++;  // 跳过 '%'
            switch (*p) {
                case 'Y':  // 4 位数的年份
                    written += snprintf(buf + written, buf_size - written, "%04d", time->tm_year + 2000);
                    break;
                case 'y':  // 2 位数的年份
                    written += snprintf(buf + written, buf_size - written, "%02d", time->tm_year);
                    break;
                case 'm':  // 2 位数的月份
                    written += snprintf(buf + written, buf_size - written, "%02d", time->tm_mon + 1);
                    break;
                case 'd':  // 2 位数的日期
                    written += snprintf(buf + written, buf_size - written, "%02d", time->tm_mday);
                    break;
                case 'H':  // 2 位数的小时
                    written += snprintf(buf + written, buf_size - written, "%02d", time->tm_hour);
                    break;
                case 'M':  // 2 位数的分钟
                    written += snprintf(buf + written, buf_size - written, "%02d", time->tm_min);
                    break;
                case 'S':  // 2 位数的秒数
                    written += snprintf(buf + written, buf_size - written, "%02d", time->tm_sec);
                    break;
                default:  // 未知占位符，直接写入
                    if (written < buf_size - 1) {
                        buf[written++] = '%';
                        buf[written++] = *p;
                    }
                    break;
            }
        } else {
            // 非占位符字符，直接写入
            buf[written++] = *p;
        }
        p++;  // 移动到下一个字符
    }

    // 添加终止符
    if (written < buf_size) {
        buf[written] = '\0';
    } else {
        // 缓冲区不足，返回 -1
        return -1;
    }

    return written;
}

/**
 * @brief 返回一个时间戳(单位是秒)
 * @param tm为null,返回当前时间戳，tm不为null，返回tm对应的时间戳
 */
time_t sys_time(tm_t *tm)
{
    if (tm == NULL)
    {
        tm_t cur;
        sys_get_clocktime(&cur);
        return mktime(&cur);
    }else{
        return mktime(tm);
    }
}


/**
 * @brief 时间初始化，获取开机时间
 */
void time_init(void)
{
    tm_t time;

    do
    {
        time.tm_sec = read_cmos(0x00);  // 秒
        time.tm_min = read_cmos(0x02);  // 分钟
        time.tm_hour = read_cmos(0x04); // 小时
        time.tm_wday = read_cmos(0x06); //星期
        time.tm_mday = read_cmos(0x07); // 日
        time.tm_mon = read_cmos(0x08);  // 月
        time.tm_year = read_cmos(0x09); // 年
    } while (time.tm_sec != read_cmos(0x00)); // 再次读取秒，确保数据一致性

    // 将 BCD 编码转换为二进制
    BCD_TO_BIN(time.tm_sec);
    BCD_TO_BIN(time.tm_min);
    BCD_TO_BIN(time.tm_hour);
    BCD_TO_BIN(time.tm_wday);
    BCD_TO_BIN(time.tm_mday);
    BCD_TO_BIN(time.tm_mon);
    BCD_TO_BIN(time.tm_year);

    time.tm_wday = (time.tm_wday-1)%7;//0表示周日
    startup_time = mktime(&time); // 将时间转换为 UNIX 时间戳
   
    return;
}
/**
 * @brief 获取当前时间
 */
int sys_get_clocktime(tm_t *time)
{
    irq_state_t state = irq_enter_protection();
    
    do
    {
        time->tm_sec = read_cmos(0x00);  // 秒
        time->tm_min = read_cmos(0x02);  // 分钟
        time->tm_hour = read_cmos(0x04); // 小时
        time->tm_mday = read_cmos(0x07); // 日
        time->tm_mon = read_cmos(0x08);  // 月
        time->tm_year = read_cmos(0x09); // 年
    } while (time->tm_sec != read_cmos(0x00)); // 再次读取秒，确保数据一致性
    irq_leave_protection(state);
    // 将 BCD 编码转换为二进制
    BCD_TO_BIN(time->tm_sec);
    BCD_TO_BIN(time->tm_min);
    BCD_TO_BIN(time->tm_hour);
    BCD_TO_BIN(time->tm_mday);
    BCD_TO_BIN(time->tm_mon);
    BCD_TO_BIN(time->tm_year);

    time->tm_mon--; // 月份从 0 开始

    return 0;
}