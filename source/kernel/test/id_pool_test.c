#include "tools/id.h"
#include "printk.h"
void id_pool_test(void){
    id_pool_t pid_pool;
    id_pool_init(0,512,&pid_pool);
    int id1 = allocate_id(&pid_pool);
    int id2 = allocate_id(&pid_pool);
    dbg_info("alloc ID: %d, %d\r\n", id1, id2);

    release_id(&pid_pool, id1);
    dbg_info("release ID: %d\r\n", id1);

    int id3 = allocate_id(&pid_pool);
    dbg_info("realloc ID: %d\r\n", id3);  // 应该得到与 id1 相同的值
    return;
}