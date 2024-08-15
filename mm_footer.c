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
#define CHUNKSIZE   (1<<12)   //4 KB == 2^12 Bytes == 2^12 / 4 words

#define MAX(x, y)   ((x) > (y)? (x) : (y))

#define PACK(size,alloc)((size) | (alloc))

#define GET(p)          (*(unsigned int *)(p))
#define PUT(p, val)     (*(unsigned int *)(p) = (val))

#define GET_SIZE(p)     (GET(p) & ~0x7)
#define GET_ALLOC(p)    (GET(p) & 0x1)
#define GET_PRE_ALLOC(p)(GET(p) & 0x2)

#define HDRP(bp)    ((char *)(bp) - WSIZE)
#define FTRP(bp)    ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp)   ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)   ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

static char *heap_listp;
static char *heap_curp;

static void *coalesce (void *bp){
    size_t prev_alloc = GET_PRE_ALLOC(HDRP(bp));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc){
        PUT(HDRP(bp), PACK(size, 2));
        return bp;
    }

    else if (prev_alloc && !next_alloc){
        // next fit
        if (NEXT_BLKP(bp) == heap_curp) heap_curp = bp;

        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 2));
        PUT(FTRP(bp), PACK(size, 2));
    }

    else if (!prev_alloc && next_alloc){
        // next fit
        if (heap_curp == bp) heap_curp = PREV_BLKP(bp);

        size += GET_SIZE(HDRP(PREV_BLKP(bp)));

        if (GET_PRE_ALLOC(HDRP(PREV_BLKP(bp)))){
            PUT(HDRP(PREV_BLKP(bp)), PACK(size, 2));
            PUT(FTRP(bp), PACK(size, 2));
        }
        // else{
        //     PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        //     PUT(FTRP(bp), PACK(size, 0));
        // }
        bp = PREV_BLKP(bp);
    }

    else {
        // next fit
        if (heap_curp == bp ||NEXT_BLKP(bp) == heap_curp) heap_curp = PREV_BLKP(bp);

        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));

        if (GET_PRE_ALLOC(HDRP(PREV_BLKP(bp)))){
            PUT(HDRP(PREV_BLKP(bp)), PACK(size, 2));
            PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 2));
        }
        // else{
        //     PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        //     PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        // }
        
        bp = PREV_BLKP(bp);
    }

    return bp;
}

static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE; // 더블 워드 정렬을 위해 2워드 배수로 반올림 & Byte 단위 변환

    if ((long)(bp = mem_sbrk(size)) == -1) return NULL;

    if (GET_PRE_ALLOC(HDRP(bp))){
        PUT(HDRP(bp), PACK(size, 2));
        PUT(FTRP(bp), PACK(size, 2));
    }
    else{
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    return coalesce(bp); 
}

/// first fit
// static void *find_fit(size_t asize){
//     char *bp = heap_listp;
    
//     while (GET_SIZE(HDRP(bp)) > 0){
//         if (GET_ALLOC(HDRP(bp)) == 0 && asize <= GET_SIZE(HDRP(bp))) return bp;
  
//         bp = NEXT_BLKP(bp);
//     }
//     return NULL;
// }

// next fit
static void *find_fit(size_t asize)
{
    char *bp = heap_curp;
    
    while (GET_SIZE(HDRP(bp)) > 0){
        if (GET_ALLOC(HDRP(bp)) == 0 && asize <= GET_SIZE(HDRP(bp))) {
            heap_curp = bp;
            return bp;
        }
     
        bp = NEXT_BLKP(bp);
    }

    bp = heap_listp;
    
    while (bp != heap_curp){
        if (GET_ALLOC(HDRP(bp)) == 0 && asize <= GET_SIZE(HDRP(bp))) {
            heap_curp = bp;
            return bp;
        }
     
        bp = NEXT_BLKP(bp);
    }

    return NULL;
}

//best fit
// static void *find_fit(size_t asize){
//     char *bp = heap_listp;
//     char *result = NULL;
//     size_t min_size = ~(size_t)0;
    
//     while (GET_SIZE(HDRP(bp)) > 0){
//         if (GET_ALLOC(HDRP(bp)) == 0 && asize <= GET_SIZE(HDRP(bp)) && GET_SIZE(HDRP(bp)) < min_size){
//             min_size = GET_SIZE(HDRP(bp));
//             result = bp;
//         }
//         bp = NEXT_BLKP(bp);
//     }

//     return result;
// }

static void place(void *bp, size_t asize){
    size_t size;
    size = GET_SIZE(HDRP(bp));

    if ((size-asize) >= DSIZE){
        if (GET_PRE_ALLOC(HDRP(bp)))
            PUT(HDRP(bp), PACK(asize, 3));
        else PUT(HDRP(bp), PACK(asize, 1));
        // PUT(FTRP(bp), PACK(asize, 1));
        
        PUT(HDRP(NEXT_BLKP(bp)), PACK(size-asize, 2));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size-asize, 2));
    }

    else{
        if (GET_PRE_ALLOC(HDRP(bp)))
            PUT(HDRP(bp), PACK(asize, 3));
        else PUT(HDRP(bp), PACK(asize, 1));
        
        PUT(HDRP(NEXT_BLKP(bp)), PACK(GET_SIZE(HDRP(NEXT_BLKP(bp))), 3));
        // PUT(FTRP(bp), PACK(size, 1));
    }
}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if ((heap_listp =  mem_sbrk(4*WSIZE)) == (void *)-1)
        return -1;
    PUT(heap_listp, 0); // unused block
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1)); // prologue block header
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1)); // prologue block footer
    PUT(heap_listp + (3*WSIZE), PACK(0, 3)); // epilogue block header
    heap_listp += (2*WSIZE); // prologue block으로 힙 ptr 설정
    heap_curp = heap_listp;

    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;

    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;
    size_t extendsize;
    char *bp;

    if (size == 0) return NULL;

    if (size <= WSIZE) asize = DSIZE;
    // else if (size <= DSIZE + WSIZE) asize = 2*DSIZE;
    else
        asize = DSIZE * ((size + WSIZE + (DSIZE-1)) / DSIZE);

    bp = find_fit(asize);

    if ((bp) != NULL){
        place(bp, asize);
        return bp;
    }

    extendsize = MAX(asize, CHUNKSIZE);
    if((bp = extend_heap(extendsize/WSIZE)) == NULL) return NULL;
    place(bp, asize);
    
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));

    if (GET_PRE_ALLOC(HDRP(ptr))){
        PUT(HDRP(ptr), PACK(size, 2));
        PUT(FTRP(ptr), PACK(size, 2));
    }

    else{ 
        PUT(HDRP(ptr), PACK(size, 0));
        PUT(FTRP(ptr), PACK(size, 0));
    }

    if (GET_ALLOC(HDRP(NEXT_BLKP(ptr))))
        PUT(HDRP(NEXT_BLKP(ptr)), PACK(GET_SIZE(HDRP(NEXT_BLKP(ptr))),1));

    else PUT(HDRP(NEXT_BLKP(ptr)), PACK(GET_SIZE(HDRP(NEXT_BLKP(ptr))),0));


    coalesce(ptr);
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
    // copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    copySize = GET_SIZE(HDRP(oldptr));
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}