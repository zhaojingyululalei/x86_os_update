#include "stdlib.h"
#include "string.h"




int main(int argc, char *argv[]) {
    
    printf("i am shell\r\n");
    uint16_t mask = umask(0);
    printf("umask =%d\r\n",mask),
    umask(mask);
    while (true)
    {
        sleep(2000);
    }
    
    return 0;
}
