
#include "stdlib.h"
#include "string.h"
int main (int argc, char ** argv);

void cstart (int argc, char ** argv,int envc,char** env) {
    int ret;
    
    // 打印 argc 和 argv 的内容
    printf("argc = %d\r\n", argc);
    printf("argv:\r\n");
    for (int i = 0; i < argc; i++) {
        printf("argv[%d] = %s\r\n", i, argv[i]);
    }

    // 打印 envc 和 env 的内容
    printf("envc = %d\r\n", envc);
    printf("env:\r\n");
    char buf[10]={0};
    for (int i = 0; i < envc; i++)
    {
        strncpy(buf,env[i],strlen(env[i]));
        printf("env[%d]=%s\r\n",i,buf);
        memset(buf,0,10);
    }
    

    // 调用 main 函数
    ret = main(argc, argv);
    exit(ret);
    
}