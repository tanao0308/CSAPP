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
    mem_sbrk(8*22);
    void* p = mem_heap_lo();
    for(int i=0; i<22; ++i) {
        *(size_t*)(p+8*i) = 0;
    }
    return 0;
}

void unpack_head(size_t head, int* size, int* tag) {
    *size = head >> 3;
    *tag = head & 7;
}

void* find_block(int idx) {
    if()
    void* p = mem_heap_lo();
    p += idx*8;
    p = *(void**)p; // 让 p 指向 p 指向地址这个地址上的值
    while(1) {
        size_t head = *(size_t*)p;
        int size, tag;
        unpack_head(head, &size, &tag);
        if()
    }
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
/*
p0, p1, p2, ..., p21
*/
void *mm_malloc(size_t size)
{
    int idx = 0;
    while((1<<idx) < size) {
        ++idx;
    }
    void* p = find_block(idx);
    return p+8;
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














