#include <unistd.h>

// 这个模型，堆内存无法下降，只能单增

// 单纯 malloc 了全部的堆内存
void mem_init(void);           
// 单纯的 free    
void mem_deinit(void);
// 堆扩展 incr 字节
void *mem_sbrk(int incr);
// 堆清空
void mem_reset_brk(void); 
// 堆起始地址
void *mem_heap_lo(void);
// 堆当前的最高地址
void *mem_heap_hi(void);
// 堆当前使用的全部地址（连续的）
size_t mem_heapsize(void);
// 返回内存页大小
size_t mem_pagesize(void);

