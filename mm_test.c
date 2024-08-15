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

void place(void *bp, size_t asize);
void *extend_heap(size_t words);
void *next_fit(size_t asize);
void *coalesce(void *bp);

//word 사이즈 정의. size_t가 운영체제에 맞게 자동변환된다.
#define WSIZE sizeof(size_t)
#define DSIZE WSIZE*2

// 사이즈 + 7. 더블워드에 맞춰서 크기를 정렬함.
// 7은 111. 비트 반전하면 000이다. and 연산을 했으므로 마지막 3자리가 0으로 바뀜.
#define ALIGN(size) (((size) + (DSIZE-1)) & ~0x7)
#define GET_SIZE(p)  (GET(p) & ~0x7)

//크기 할당용, 책의 경우 1<<12 즉, 4KB
#define CHUNKSIZE (1<<12)  /* 메모리 페이지 단위와 동일. 물리 메모리와 맞춰서 매핑이 더 잘되게끔 함 */

//전역 변수로 malloc의 시작점과 끝점 저장하기. 포인터가 되어야할듯.
static char *heap_listp = NULL; //시작점
static char *rover = NULL; //지금 중간점

//포인터포인터형변환 정신없으니까 사용하는게 좋음
#define GET(p)       (*(size_t *)(p)) //포인터가 가리키는 내용을 가져오는 매크로   
#define PUT(p, val)  (*(size_t *)(p) = (val)) //포인터가 가리키는 곳에 저장


//header와 footer 위치 구함
// footer 위치는 지금위치에서 사이즈 만큼 간 것. 사이즈는 블럭 - 헤더푸터
// 블럭 사이즈는? header에 저장되어있음. (header 구하는게 더 간단)
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

//이전 블록, 다음 블록 구하는 함수
#define NEXT_BLKP(bp) ((char *)bp + GET_SIZE((char *)(bp) - WSIZE))
#define PREV_BLKP(bp) ((char *)bp - GET_SIZE((char *)(bp) - DSIZE))



int mm_init(void)
{
    //malloc의 초기 블록 생성. malloc으로 운용할 영역.
    //만약 힙에 해당 공간이 없다면, 오류 반환.
    if((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1){
        //어차피 확장인데 mem_sbrk 안써도 될듯?
        //sbrk로 대체했더니 lies outside heap 오류남.
        //만약 -1이라면, 공간이 없는 것이므로 생성 실패. -1 반환.
        return -1;
    }

    //-1이 아니라면 공간이 있는 것이므로, 초기화 작업 시작

    //주소값...채워야하므로.... alignment padding 만들어주기
    //1워드 크기만큼 맨 앞에 추가한다.
    PUT(heap_listp, 0); //무조건 1워드로 저장되기때문에 0 넣어도 됨.

    //prolog epilogue 만들기
    PUT(heap_listp+WSIZE, (DSIZE|1));
    PUT(heap_listp+(2*WSIZE), (DSIZE|1));
    PUT(heap_listp+(3*WSIZE), 1);
    heap_listp += DSIZE; //힙 시작점
    rover = heap_listp; //블록 시작점

    //블록 확장
    //원하는 메모리 용량을(바이트) 지금 운영체제 1워드로 나눠서 몇칸 필요한지 넘김
    //나중에 다시 또 곱하긴 하는데, 늘릴 사이즈가 워드 단위가 아닐 때도 있으니까 해야함.
    if(extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;      
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the DSIZE.
 */
void *mm_malloc(size_t size)
{
    //선언된 적 없다면. 초기값 생성
    if (heap_listp == NULL){
        mm_init();
    }

    if(size == 0)
        return NULL; //담을것도 없음
    
    size_t asize = ALIGN(size) + DSIZE; //사이즈 8바이트 정렬
    char *bp = next_fit(asize);

    if(bp != NULL){
        //만약 들어갈 자리가 있다면
        //자리에 맞게 분할
        place(bp, asize);
        return bp;
    }

    //만약 못 찾았다면 확장해야함
    //확장 크기랑 새로 필요한 공간중에 큰 쪽으로 정함
    size_t extendsize = (asize > CHUNKSIZE) ? asize : CHUNKSIZE;  
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)  
       return NULL; //할당불가. 
    place(bp, asize); //새공간에서 필요한 만큼 가져옴
    return bp;
}

void mm_free(void *ptr)
{
    if(ptr == 0) 
       return;

    if(heap_listp== 0)
        mm_init();

    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr), (size & ~0x1));
    PUT(FTRP(ptr), (size & ~0x1));
    coalesce(ptr);
    
    return;
}

void *mm_realloc(void *ptr, size_t size)
{
    //기존 사이즈 저장
    size_t oldsize = GET_SIZE(HDRP(ptr));
    void *newptr;
    
    if(size == 0) {
       mm_free(ptr);
       return 0;
    }
    
    //주소가 없으면 그냥 새로 사이즈만큼 할당해서 줌
    if(ptr == NULL)
        return mm_malloc(size);

    newptr = mm_malloc(size); //새 크기만큼 공간 찾아서 할당

    if(!newptr)
        return 0; //공간부족


    if(size <oldsize)
        oldsize = size; //사이즈 축소 시 옮길 값 줄임
    
    memcpy(newptr, ptr, oldsize); //복사해서 옮김

    mm_free(ptr); //기존공간 free

    return newptr; //새포인터 돌려줌
}

//가용 블록 연결 함수
void *coalesce(void *bp){

    //일단... 이전 블록, 다음 블록이 가용 블록인지 확인
    //이렇게 하면 p랑 n에는 0이나 1이 담김.
    size_t p = (GET(FTRP(PREV_BLKP(bp)))&0x1);
    size_t n = (GET(HDRP(NEXT_BLKP(bp)))&0x1);
    size_t size = GET_SIZE(HDRP(bp));

    if(p && n){ //둘다 가용블록이 아님.
        return bp;
    }else if (p && !n){ //다음 블록만 가용 블록임
        size += GET_SIZE(HDRP(NEXT_BLKP(bp))); //다음 블록이랑 합침
        PUT(HDRP(bp), size);
        PUT(FTRP(bp), size); //헤더 이미 업데이트 했으므로, 푸터 이렇게하면 다음 블록의 푸터였던 것이 합쳐짐
    }else if (!p && n){ //이전 블록만 가용 블록임
        size += GET_SIZE(HDRP(PREV_BLKP(bp))); //다음 블록이랑 합침
        PUT(FTRP(bp), size);
        PUT(HDRP(PREV_BLKP(bp)), size); //이전 블록이랑 합침. 이전 블록의 헤더는 찾아줘야함.
        bp = PREV_BLKP(bp);
    }else{ //둘다 가용블록임
        size += GET_SIZE(HDRP(NEXT_BLKP(bp))) + GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), size);
        PUT(FTRP(NEXT_BLKP(bp)), size);
        bp = PREV_BLKP(bp);
    }

    //만약 가리키고 있던 곳이 새로 합쳐서 만들어진 블록 중간에 있다면 포인터 수정
    if((rover > (char *)bp)&&(rover < NEXT_BLKP(bp)))
        rover = bp;    
    
    return bp;
}

void *next_fit(size_t asize){

    char *oldrover = rover;

    while(GET_SIZE(HDRP(rover))>0){
        //헤더에 담긴 사이즈가 0보다 클때까지
        //즉, 에필로그 헤더에 도착할때까지
        if(!(GET(HDRP(rover))&0x1)) //가용 블록일 것
            //해당 크기를 넣을 수 있을 것
            if(GET_SIZE(HDRP(rover)) >= asize)
                return rover;

        rover = NEXT_BLKP(rover);
    }

    rover = heap_listp;

    //오른쪽에서 못찾았으므로 왼쪽에서 찾아야함
    //주소값이 시작점보다 작을때까지
    while(rover < oldrover){
        if(!(GET(HDRP(rover))&0x1))
            if(GET_SIZE(HDRP(rover)) >= asize)
                return rover;
        rover = NEXT_BLKP(rover);
    }

    //못찾음
    return NULL;
}

//malloc heap 확장
void *extend_heap(size_t words){
    char *bp;
    size_t size=words*WSIZE;

    //홀수면 워드 하나 더 더해줌
    if(size%2 == 1)
        size += WSIZE;

    //새로운 영역 할당
    if((size_t)(bp = mem_sbrk(size)) == -1)
        return NULL;

    //if문 통과했으면 할당 잘 됨. 새로운 영역을 기존 영역과 합침
    //참고로 이전 에필로그 헤더를 같이 합치기 때문에, size에서 따로 에필로그 헤더 영역을 빼지 않아도 된다.
    //프롤로그 헤더는 기존걸 사용하므로 마찬가지로 뺄 필요 x
    
    //기존 epilogue header 가 새로운 일반블록의 header가 됨
    PUT(HDRP(bp), size);
    PUT(FTRP(bp), size); //푸터도 만들어줌
    PUT(HDRP(NEXT_BLKP(bp)), 1); //new epilogue header

    return coalesce(bp); //합친 뒤 돌아감
}

//가용공간 분할해서 할당해주는 함수
void place(void *bp, size_t asize){

    //일단 기존 공간 크기 저장
    size_t csize = GET_SIZE(HDRP(bp));

    if((csize-asize) >= (2*DSIZE)){
        //기존공간이 필요한 공간을 할당하고도 한블록 더 생길수있을 공간인가?
        //그렇다면 분할

        //기존 공간에 지금 블록 사이즈를 넣음
        PUT(HDRP(bp), (asize| 1));
        PUT(FTRP(bp), (asize| 1));

        //다음 블록 계산
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), (csize-asize));
        PUT(FTRP(bp), (csize-asize));
    }else{
        //공간이 없다면 그냥 할당
        PUT(HDRP(bp), (csize| 1));
       PUT(FTRP(bp), (csize| 1));
    }
}