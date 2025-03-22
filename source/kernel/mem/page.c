#include "mem/page.h"
#include "mem/bitmap.h"
#include "mem/kmalloc.h"
#include "mem/pdt.h"
#include "algrithm.h"
#include "cpu_cfg.h"
#include "printk.h"
#include "irq/irq.h"
static page_t *page_buffer;
static uint32_t page_arr_len;
static list_t page_free_list; // page_t空闲链表，用于分配page_t
static rb_tree_t page_tree;
static int page_compare(const void *a, const void *b)
{
    int a_addr = ((page_t *)a)->phaddr;
    int b_addr = ((page_t *)b)->phaddr;

    return a_addr - b_addr;
}
static rb_node_t *page_get_node(const void *data)
{
    return &((page_t *)data)->rbnode;
}
static page_t *node_get_page(rb_node_t *node)
{
    page_t *s = rb_node_parent(node, page_t, rbnode);
    return s;
}
static int compare_phaddr(const void *key, const void *data)
{
    ph_addr_t key_phaddr = (ph_addr_t)key;
    page_t *s = (page_t *)data;
    return key_phaddr - s->phaddr;
}

void page_manager_init(void)
{
    int ret;
    uint32_t page_cnt = calc_bitmap_pages(); // 管理多少页
    // page_tbuffer占多大内存
    uint32_t page_buffer_len_byte = align_up(page_cnt * sizeof(page_t), MEM_PAGE_SIZE);
    // 给page_t buffer分配内存
    page_buffer = (page_t *)mm_bitmap_alloc_pages(page_buffer_len_byte / MEM_PAGE_SIZE);
    // page_t数组长度
    page_arr_len = page_cnt;

    list_init(&page_free_list);
    // 全部放入空闲链表等待分配
    for (uint32_t i = 0; i < page_cnt; i++)
    {
        list_insert_last(&page_free_list, &page_buffer[i].lnode);
    }

    dbg_info("page_T free list count %d\r\n", list_count(&page_free_list));
    // 初始化page_tree存放所有page页
    rb_tree_init(&page_tree, page_compare, page_get_node, node_get_page,NULL);
    // kmalloc弄好后，在这里把kmalloc管理的区域记为内核页
    
    ph_addr_t ph_start = get_km_allocator_start();
    size_t size = get_km_allocator_size();
    ret = record_continue_pages(NULL,ph_start,size,PAGE_TYPE_KERNEL);
    if(ret <0 ){
        dbg_error("km allocator init fail\r\n");

    }
}

page_t *page_t_alloc()
{
    list_node_t *node = list_remove_first(&page_free_list);
    page_t *ret = list_node_parent(node, page_t, lnode);
    return ret;
}

void page_t_free(page_t *page)
{
    memset(page, 0, sizeof(page_t));
    list_insert_last(&page_free_list, &page->lnode);
}

/**
 * @brief 创建一个页，谁创建的，创建的什么页,然后插到树上
 */
int create_one_page(task_t *task, ph_addr_t phaddr, vm_addr_t vmaddr, page_type_t type)
{
    page_t *page = page_t_alloc();
    page->type = type;
    page->phaddr = phaddr;

    // 留着，先写kmalloc吧，要不然总得建立内存池太麻烦
    page_owner_t *owner = kmalloc(sizeof(page_owner_t));
    owner->task = task;
    owner->vmaddr = vmaddr;

    list_insert_last(&page->onwers_list, &owner->node);
    page->refs = 1;
    rb_tree_insert(&page_tree,page);
    return 0;
}
/**
 * @brief 找一个物理页，如果存在就返回。
 */
static page_t *find_one_page(ph_addr_t phaddr)
{
    rb_node_t *node = rb_tree_find_by(&page_tree, phaddr, compare_phaddr);
    page_t *page = NULL;
    if (node==page_tree.nil)
    {
        page = NULL;
        
    }
    else
    {
        page = node_get_page(node);
    }
    return page;
}
/**
 * @brief 记录一个页
 */
int record_one_page(task_t *task, ph_addr_t phaddr, vm_addr_t vmaddr, page_type_t type)
{
    ASSERT(phaddr%MEM_PAGE_SIZE==0);
    ASSERT(vmaddr %MEM_PAGE_SIZE == 0);
    irq_state_t state = irq_enter_protection();
    
    page_t *page = find_one_page(phaddr);
    // 如果找到了就refs++,没找到创建一个page插入树中
    if (page)
    {
        if(page->type != type){
            dbg_error("page type and record type not matach,record fail\r\n");
            ASSERT(page->type == type);
        }
        //添加拥有者
        page_owner_t *owner = kmalloc(sizeof(page_owner_t));
        owner->task = task;
        owner->vmaddr = vmaddr;
        list_insert_last(&page->onwers_list,&owner->node);
        //引用++
        page->refs++;
    }
    else
    {
        //没找到就创建一个页
        create_one_page(task,phaddr,vmaddr,type);
    }
    irq_leave_protection(state);
    return 0;
}

/**
 * @brief 记录一段连续的页
 */
int record_continue_pages(task_t* task,vm_addr_t vmstart,size_t size,page_type_t type){ 
    page_entry_t* page_table;
    if(!task){
        page_table = get_global_page_table();
    }else{
        page_table = task->page_table;
    }
    int cnt = size/MEM_PAGE_SIZE;
    for (int i = 0; i < cnt; i++)
    {
        vm_addr_t vm = vmstart+i*MEM_PAGE_SIZE;
        ph_addr_t ph = vm_to_ph(page_table,vm);
        record_one_page(task,ph,vm,type);
    }
    return 0;
}
/**
 * @brief 找到一个页的owner，通过task找。
 * @note 通过vm找，可能找到多个页。通过task找肯定只有一个
 */
static page_owner_t* find_page_owner_by_task(page_t* page,task_t* task){
    list_t* list = &page->onwers_list;
    list_node_t* cur = list->first;
    while (cur)
    {
        page_owner_t* owner = list_node_parent(cur,page_owner_t,node);
        if(owner->task == task){
            return owner;
        }
        cur = cur->next;
    }
    return NULL;
    
}

/**
 * @brief 删除一个页
 */

int remove_one_page(task_t* task,vm_addr_t vm_addr){
    ASSERT(vm_addr % MEM_PAGE_SIZE == 0);
    irq_state_t state = irq_enter_protection();
    
    //通过映射，获取物理地址
    ph_addr_t phaddr = vm_to_ph(task->page_table,vm_addr);
    //找到对应的页结构
    rb_node_t *node = rb_tree_find_by(&page_tree,phaddr,compare_phaddr);
    page_t* page = NULL;
    if(node==page_tree.nil){
        dbg_error("the page vm=0x%x not record in page_tree\r\n",vm_addr);
        irq_leave_protection(state);
        return -1;
    }else{
        page = node_get_page(node);
    }
    page_owner_t* owner = find_page_owner_by_task(page,task);
    if(!owner){
        dbg_error("task:%d vm:0x%x not in the ownerlist\r\n",task->pid,vm_addr);
        irq_leave_protection(state);
        return -2;
    }
    list_remove(&page->onwers_list,&owner->node);
    page->refs--;
    if(page->refs==0){
        //bitmap相应位清0
        mm_bitmap_free_page(phaddr);
        //从树上移除
        rb_tree_remove(&page_tree,page);
        //释放page_t
        page_t_free(page);
    }
    irq_leave_protection(state);
    return 0;

}
/**
 * @brief 删除很多页
 */
int remove_pages(task_t* task,vm_addr_t vmstart,size_t size){
    int cnt = size/MEM_PAGE_SIZE;
    int ret;
    for (int i = 0; i < cnt; i++)
    {
        ret = remove_one_page(task,vmstart+i*MEM_PAGE_SIZE);
        ASSERT(ret == 0);
    }
    return 0;
    
}