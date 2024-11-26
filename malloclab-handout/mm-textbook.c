/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

struct Block {
    size_t size;
    char empty;
} __attribute__((aligned(8)));

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    mem_init();
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
/*
先实现一个每次从头开始找空间大于自身的空闲块的方法
将每个数据块分为自定义的 struct Block 头部和 data 数据部
头部存的 size 字段是 data 对齐后的大小（对齐之前的大小并无意义）
*/
void *mm_malloc(size_t size)
{
    void* p = mem_heap_lo();
    while(p <= mem_heap_hi()) {
        struct Block* block = (struct Block*)p;
        if(block->empty == 0 || size > block->size) {
            // 这里的 struct Block 的大小貌似已经是按照自动对齐之后的大小计算的了，而且 block->size 存的也是对齐之后的值
            p = p + sizeof(struct Block) + block->size;
            continue;
        }
        // 若能够分拆，则分拆为两个块
        if(block->size >= ALIGN(size) + sizeof(struct Block) + 8) {
            struct Block* new_block = (struct Block*)(p + sizeof(struct Block) + ALIGN(size));
            new_block->size = block->size - ALIGN(size) - sizeof(struct Block);
            new_block->empty = 1;
            block->size = ALIGN(size);
            block->empty = 0;
        } else { // 否则，直接存入
            block->empty = 0;
        }
        return p + sizeof(struct Block);
    }

    p = mem_sbrk(sizeof(struct Block) + ALIGN(size));
    if (p == (void *)-1)
	    return NULL;
    else {
        struct Block* block = (struct Block*)p;
        block->size = ALIGN(size);
        block->empty = 0;
        return p + sizeof(struct Block);
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    struct Block* block = (struct Block*)(ptr - sizeof(struct Block));
    block->empty = 1;
    
    void* p = mem_heap_lo();
    while(1) {
        struct Block* block = (struct Block*)p;
        if(p + sizeof(struct Block) + block->size > mem_heap_hi()) {
            break;
        }
        void* nxt_p = p + sizeof(struct Block) + block->size;
        struct Block* nxt_block = (struct Block*)nxt_p;
        if(!block->empty || !nxt_block->empty) {
            p += sizeof(struct Block) + block->size;
            continue;
        }
        block->size += sizeof(struct Block) + nxt_block->size;
    }
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}














