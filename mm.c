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
    "Minkyoung Kim",
    /* First member's email address */
    "kina12345600@gmail.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8   

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE       4
#define DSIZE       8
#define CHUNKSIZE   ((1<<12) / WSIZE)   //4 KB == 2^12 Bytes == 2^12 / 4 words

#define MAX(x, y) ((x) > (y)? (x) : (y))

#define PACK(size,alloc) ((size) | (alloc))

#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = val)

#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

#define HDRP(bp)    ((char *)(bp) - WSIZE)
#define FTRP(bp)    ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp)   (((char *)(bp) + GET_SIZE((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)   (((char *)(bp) - GET_SIZE((char *)(bp) - DSIZE)))

static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE; // 더블 워드 정렬을 위해 2워드 배수로 반올림 & Byte 단위 변환
    bp = mem_sbrk(size);

    if ((long)(bp) == -1) return NULL;

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    return coalesce(bp);
    return bp;    
}
/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{

    void *heap_listp =  mem_sbrk(4*WSIZE);
    if (heap_listp == (void *)-1)
        return -1;
    PUT(heap_listp, 0); // unused block
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1)); // prologue block header
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1)); // prologue block footer
    PUT(heap_listp + (3*WSIZE), PACK(DSIZE, 1)); // epilogue block header
    heap_listp += (2*WSIZE); // prologue block으로 힙 ptr 설정

    if (extend_heap(CHUNKSIZE) == NULL)
        return -1;

    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);    // mem_brk를 size만큼 이동하고 이전 포인터값 반환, 음수 size 입력이나 메모리 공간 초과 시 -1 반환
    if (p == (void *)-1) return NULL;
    else {
        *(size_t *)p = size;    
        return (void *)((char *)p + SIZE_T_SIZE);
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    // coalesce(bp);
}

static void *coalesce(void *bp){
    size_t prev_alloc; 
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














