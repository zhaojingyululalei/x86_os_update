#ifndef __SIGNAL_H
#define __SIGNAL_H


#include "types.h"
#include "cpu_cfg.h"
#define SIGNAL_MAX_CNT  32
// 终止 / 中断信号
#define SIGHUP     1   // 挂起（终端关闭）
#define SIGINT     2   // 中断（Ctrl+C）
#define SIGQUIT    3   // 退出（Ctrl+\）
#define SIGILL     4   // 非法指令
#define SIGTRAP    5   // 调试陷阱
#define SIGABRT    6   // 异常终止
#define SIGBUS     7   // 总线错误（非法内存访问）
#define SIGFPE     8   // 浮点异常（除0、溢出等）
#define SIGKILL    9   // 立即终止（不可捕获）
#define SIGUSR1    10  // 用户自定义信号1
#define SIGSEGV    11  // 段错误（无效内存引用）
#define SIGUSR2    12  // 用户自定义信号2
#define SIGPIPE    13  // 向无读端管道写数据
#define SIGALRM    14  // 闹钟（由 alarm() 触发）
#define SIGTERM    15  // 终止（可捕获，可阻塞）
#define SIGSTKFLT  16  // 协处理器栈错误（几乎不用）
#define SIGCHLD    17  // 子进程状态改变
#define SIGCONT    18  // 继续执行（如果被停止）
#define SIGSTOP    19  // 停止进程（不可捕获）
#define SIGTSTP    20  // 控制终端停止（Ctrl+Z）
#define SIGTTIN    21  // 后台进程读取终端输入
#define SIGTTOU    22  // 后台进程写终端输出
#define SIGURG     23  // 套接字紧急数据到达
#define SIGXCPU    24  // 超出 CPU 时间限制
#define SIGXFSZ    25  // 文件大小超过限制
#define SIGVTALRM  26  // 虚拟定时器到时
#define SIGPROF    27  // Profiling 定时器到时
#define SIGWINCH   28  // 窗口大小改变
#define SIGIO      29  // I/O 事件通知
#define SIGPWR     30  // 电源故障
#define SIGSYS     31  // 非法系统调用（bad syscall）



typedef void (*signal_handler)(int);
enum sig_how{
    SIG_BLOCK,
    SIG_UNBLOCK,
    SIG_SETMASK,
};
typedef struct _sigset_t{
    
    uint32_t bitmap;
}sigset_t;
void signal_init(void);

int sys_signal(int signum, signal_handler handler);
int sys_sigpromask(int how, const sigset_t *set, sigset_t *old);
int sys_raise(int signum);
int sys_kill(int pid, int signum);
int sys_sigpending(sigset_t *set);
int sys_pause(void);


#endif