/*
 * mm-naive.c - 가장 빠르지만 메모리 효율은 가장 낮은 malloc 패키지.
 *
 * 이 단순한 방식에서는 brk 포인터를 단순히 증가시켜 block을 할당합니다.
 * block은 순수한 payload만으로 이루어지며 header나 footer가 없습니다.
 * block은 coalescing되거나 재사용되지 않습니다. Realloc은
 * mm_malloc과 mm_free를 직접 사용해 구현합니다.
 *
 * 학생에게 알림: 이 header comment는 여러분의 해법을 높은 수준에서
 * 설명하는 내용으로 교체하세요.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * 학생에게 알림: 다른 작업보다 먼저 아래 struct에
 * 팀 정보를 입력하세요.
 ********************************************************/
team_t team = {
    /* 팀 이름 */
    "ateam",
    /* 첫 번째 팀원의 전체 이름 */
    "Harry Bovik",
    /* 첫 번째 팀원의 이메일 주소 */
    "bovik@cs.cmu.edu",
    /* 두 번째 팀원의 전체 이름 (없으면 비워 두기) */
    "",
    /* 두 번째 팀원의 이메일 주소 (없으면 비워 두기) */
    ""};


// /* single word(4) 또는 double word(8) alignment */
// #define ALIGNMENT 8

// /* ALIGNMENT의 가장 가까운 배수로 올림 */
// #define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7) /* ~0x7 = 1111 1000 */

// #define SIZE_T_SIZE (ALIGN(sizeof(size_t)))


/* 내가 정의한 매크로 */
#define WSIZE 4   /* 워드, 헤더/푸터 크기 (바이트) */
#define DSIZE 8   /* 더블 워드 크기 (바이트) */
#define CHUNKSIZE (1<<12)   /* 힙 확장 기본 크기(힙 확장할때 얼마나 늘릴지) */

#define GET(p) (*(unsigned int *)(p))   /* p주소(포인터)에서 4바이트 읽기: 헤더, 푸터 사이즈 */
#define PUT(p, val) (*(unsigned int *)(p) = (val)) /* p주소(포인터)에서 4바이트 쓰기: 헤더, 푸터 사이즈 */

#define PACK(size, alloc) ((size) | (alloc)) /* size와 alloc을 합쳐서 헤더/푸터 값 생성 */
#define GET_SIZE(p) (GET(p) & ~0x7) /* 헤더/푸터에서 블록 크기(size) 추출 */
#define GET_ALLOC(p) (GET(p) & 0x1) /* 헤더/푸터에서 할당 비트(alloc) 추출 */

#define HDRP(bp) ((char *)(bp) - WSIZE) /* bp의 헤더 주소 반환 */
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) /* bp의 푸터 주소 반환 */

/* bp = 페이로드 시작주소, 일관성있게 bp 기준으로 동작하기때문에 아래 함수도 페이로드 시작주소(bp)를 반환함 */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE((char *)(bp) - WSIZE)) /* 다음 블록의 bp로 이동*/
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE((char *)(bp) - DSIZE)) /* 이전 블록의 bp로 이동 */

#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* exp 구현: 이전, 이후 포인터 매크로 */
#define PREV_FREE(bp) (*(char **)(bp))
#define NEXT_FREE(bp) (*(char **)((char *)(bp) + DSIZE))


static char *heap_listp; /* 힙 시작 포인터 */
static char *free_listp; /* free 리스트 시작 포인터 */


/* 전방 선언 */
static void *coalesce(void *bp);
static void *extend_heap(size_t words);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);
static void add_free(char *bp);
static void remove_free(char *bp);


/* free 블록을 처리하는 함수
 * 그러므로 양쪽이 alloc 되어있대도 나(bp)는 free
 */
static void *coalesce(void *bp)
{
    /* alloc 뽑기 - 헤더값, 푸터값 주소 선택 - 이전블럭, 이후블럭 선택, 모두 매크로에 구현되어있으니 사용해야함 */
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    /* PUT으로 헤더/푸터를 새로 써주고, 현재 사이즈에서 추가 사이즈 더해주기 */
    if (prev_alloc && next_alloc) {           /* case 1: 양쪽 다 allocated - 합칠 거 없음 */
        add_free(bp); // 합칠 블록이 없으니 bp만 free 리스트에 추가
        return bp;
    }
    else if (prev_alloc && !next_alloc) {     /* case 2: 다음 블록이랑 현재 블록을 합침 */
        remove_free(NEXT_BLKP(bp)); // + 다음 블록을 리스트에서 제거 (합칠거니까)
        size += GET_SIZE(HDRP(NEXT_BLKP(bp))); /* 현재 블럭의 크기에 더해진 다음 블럭의 크기 추가해주기 */
        PUT(HDRP(bp), PACK(size, 0));           /* 헤더 추가, 그냥 지금 블록 위치 앞에 */
        /* 헤더가 이미 업데이트 됐으므로 FTRP(NEXT_BLKP(bp))가 아니라 FTRP(bp)를 써도 된다. */
        PUT(FTRP(bp), PACK(size, 0));           /* 푸터: 합쳐진 다음 블록의 푸터 위치에 */
    }
    else if (!prev_alloc && next_alloc) {     /* case 3: 이전 블록이랑 현재 블록을 합침 */
        /* bp를 먼저 업데이트: PREV_BLKP 반복 호출 시 포인터 오염 방지 */
        /* 먼저 업데이트하면 PREV_BLKP(bp)로 매번 계산하는게 아니라 bp만 호출하면 된다. */
        bp = PREV_BLKP(bp);
        remove_free(bp); // + 이전 블록을 리스트에서 제거
        size += GET_SIZE(HDRP(bp)); /* 블록 크기는 무조건 헤더에서 읽어와야함 */
        PUT(HDRP(bp), PACK(size, 0)); /* 헤더: 이전 블럭 */
        PUT(FTRP(bp), PACK(size, 0)); /* 푸터: 합쳐진 블록의 푸터 */
    }
    else {                                        /* case 4: 양쪽 다 free */
        remove_free(PREV_BLKP(bp));
        remove_free(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        size += GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    add_free(bp); // 전부 합쳐진 블록을 free 리스트에 추가
    return bp;
}


static void *extend_heap(size_t words)
{
    char *bp; // 확장 후 bp 주소 저장
    size_t size; // 실제 확장할 바이트 수를 저장

    // 4의 짝수곱은 늘 8의 배수
    // 조건 ? 참일때 값 : 거짓일떄 값
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;

    /* 1. 힙을 words 만큼 확장 */
    bp = mem_sbrk(size); // 확장 size만큼 bp에 저장
    if (bp == (void *)-1)
        return NULL; // 반환값 void: 실패 시 NULL 반환

    /* 2. 새로 확장된 공간을 free 공간으로 초기화 - PACK으로 */
    PUT(HDRP(bp), PACK(size, 0)); // 새 free 블록 헤더
    PUT(FTRP(bp), PACK(size, 0)); // 새 free 블록 푸터
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); // 에필로그 헤더, 새 free 다음 블록에 위치: 힙의 새 끝

    return coalesce(bp); /* 3. 인접한 free 블록이 있다면 coalescing(확장) */
}





/* first fit: 특정 블록을 탐색하는게 아니라 힙 전체를 순회해야한다.
 * 순회 시작점: 첫번째 블록의 bp(heap_listp), 순회 끝: 에필로그 헤더를 만났을 때
 * 에필로그 헤더가 size=0인걸 활용해서 순회를 멈추면 됨
 * 
 * 즉, find_fit은 asize 이상인 free 블록을 찾는 함수야.
 */
static void *find_fit(size_t asize)
{
    void *bp;
    // for (초기값; 조건; 증감)
    // free 리스트만 순회
    for (bp = free_listp; bp != NULL; bp = NEXT_FREE(bp)) {
        if (GET_SIZE(HDRP(bp)) >= asize)
            return bp;
    }
    return NULL;
}


/* place: place는 find_fit으로 찾아온 블록을 실제로 할당 처리하는 함수
 * 할당 후 필요하면 분할
 * 분할을 하든 안하든 free 리스트에서 제거해줘야함. 할당하면 free가 아니기때문
 */
static void place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp)); // 현재 블록 크기
    remove_free(bp); // 할당 되었으면 free list에서 제거

    // 남은 공간(현재 블록 크기 - asize)이 최소블록크기(2*DSIZE)이상이면 분할
    if (csize - asize >= 3 * DSIZE)
    {
        // 분할 후 메모리 공간 변화를 PUT으로 하나하나 써줘야함
        /* 1. 앞 블록 헤더, 기존 헤더 위치에 덮어씀 */
        PUT(HDRP(bp), PACK(asize, 1));
        /* 2. 앞 블록 푸터 */
        PUT(FTRP(bp), PACK(asize, 1));

        /* 다음블록 지정 매크로 쓰기, 남은 크기만 할당하기, alloc 하기 */
        /* 3. 뒷 블록 헤더 */
        PUT(HDRP(NEXT_BLKP(bp)), PACK(csize - asize, 0));
        /* 4. 뒷 블록 푸터 */
        PUT(FTRP(NEXT_BLKP(bp)), PACK(csize - asize, 0));
        /* 5. 분할 후 남은 블럭 공간 free list 추가 */
        add_free(NEXT_BLKP(bp));
    } else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}



/*
 * mm_init - malloc 패키지를 초기화합니다.
 */
int mm_init(void)
{
    free_listp = NULL; // + 처음에는 free 블록이 없음
    heap_listp = mem_sbrk(4 * WSIZE); // 1. 초기 힙 정의, 크기 할당 (힙 시작 주소에)
    
    if (heap_listp == (void *)-1) // 2. mem_sbrk 함수가 실패 처리 코드를 내보냈을 때
        return -1; // 함수 반환타입이 int이므로 실패 -1

    PUT(heap_listp, 0); // 1. 패딩
    PUT(heap_listp + WSIZE, PACK(DSIZE, 1)); // 2. 프롤로그 헤더(size는 프롤로그 블록 크기 넣어야함)
    PUT(heap_listp + WSIZE * 2, PACK(DSIZE, 1)); // 3. 프롤로그 푸터
    PUT(heap_listp + WSIZE * 3, PACK(0, 1)); // 4. 에필로그 헤더

    heap_listp += DSIZE;  // 프롤로그 헤더 다음으로 이동 → 프롤로그 푸터 시작점 = 첫 블록의 bp

    // 초기 힙은 생성했으나 가용 블록이 없으므로 힙 블록 확장
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;
    
    return 0;
}

/*
 * mm_malloc - brk 포인터를 증가시켜 block을 할당합니다.
 *     항상 alignment의 배수 크기를 갖는 block을 할당합니다.
 */
void *mm_malloc(size_t size)
{
    size_t asize; // 정렬된 블록 크기
    size_t extendsize; // 힙 확장 크기 
    char *bp;

    /* 1. size 조정: size가 0인경우, 잘못된 요청 */
    if (size == 0)
        return NULL;

    /* 2. size → asize로 조정 */
    if (size <= DSIZE) {
        // DSIZE 로만 설정하면 헤더, 푸터만 위치할 수 있고 공간이 끝남
        // 최소한의 payload를 포함할 공간이 필요함 그래서 2 * DSIZE로 기본 값을 맞춰줘야함
        asize = 3 * DSIZE;
    } else {
        /* size > DSIZE 일때 8의 배수로 맞춰주는 분기식 */
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
        if (asize < 3 * DSIZE) asize = 3 * DSIZE;
    }
    
    bp = find_fit(asize);                        /* free 블록 탐색 */
    if (bp == NULL) {                            /* 맞는 free 블록 없으면 */
        extendsize = MAX(asize, CHUNKSIZE);      /* 확장 크기 결정 (asize와 CHUNKSIZE 중 큰 값) */
        bp = extend_heap(extendsize / WSIZE);   /* 힙 확장 (바이트 → 워드 변환) */
        if (bp == NULL)                          /* 힙 확장도 실패하면 */
            return NULL;                         /* NULL 반환 */
    }
    place(bp, asize);                            /* 찾은 블록에 할당 */
    return bp;                                   /* 할당된 블록 반환 */
    
    
}



/*
 * mm_free - block을 해제해도 아무 일도 하지 않습니다.
 */
void mm_free(void *ptr)
{
    // PUT(ptr, 0): ptr 위치에 그냥 0을 쓰는것. 
    // 헤더랑 푸터의 alloc 비트만 0으로 바꾸고 size는 그대로 유지
    // ptr은 bp, 페이로드 시작점이기 때문에 헤더값 가져오려면 HDRP 꼭 써야함
    size_t size = GET_SIZE(HDRP(ptr)); /* 현재 블록 크기 */
    PUT(HDRP(ptr), PACK(size, 0)); /* 헤더 alloc 0으로 */
    PUT(FTRP(ptr), PACK(size, 0)); /* 푸터 alloc 0으로 */
    coalesce(ptr);
}

/*
 * mm_realloc - mm_malloc과 mm_free를 이용해 단순하게 구현합니다.
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    if (ptr == NULL)
    return mm_malloc(size);

    if (size == 0) {
        mm_free(ptr);
        return NULL;
    }

    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    copySize = GET_SIZE(HDRP(oldptr));
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}


/* explicit list 함수 추가 */
static void add_free(char *bp)
{
    // 1. 새블록의 next = 기존 free_listp
    // NEXT_FREE는 읽는 매크로, NEXT_FREE(bp)가 bp의 next 포인터 위치이므로 거기에 값을 대입하면 됨
    NEXT_FREE(bp) = free_listp;
    // 2. 새블록의 prev = NULL
    PREV_FREE(bp) = NULL;
    // 3. 블록A의 prev = 새블록 (블록A가 NULL이 아닐 때만)
    // 블록 A: 기존 free_listp가 가리키던 블록 (이전 free 블록)
    if (free_listp != NULL)
        PREV_FREE(free_listp) = bp;  // 블록A의 prev = 새블록
    // 4. free_listp = 새블록
    free_listp = bp;      // free_listp = 새블록 (맨 앞으로 업데이트)

}


static void remove_free(char *bp)
{
    // 1. bp의 prev가 NULL이면 (bp가 맨 앞)
    if (PREV_FREE(bp) == NULL) {
        // free_listp = bp의 next
        free_listp = NEXT_FREE(bp);
    } else {
        // 2. bp의 prev가 있으면
        // 블록A(기준 블록의 앞블록)의 next = bp의 next
        NEXT_FREE(PREV_FREE(bp)) = NEXT_FREE(bp);
    }

    // 3. bp의 next가 NULL이면 (bp가 맨 뒤) → 아무것도 안 해도 됨
    // 4. bp의 next가 있으면 → 블록B의 prev = bp의 prev
    if (NEXT_FREE(bp) != NULL) {
        PREV_FREE(NEXT_FREE(bp)) = PREV_FREE(bp);
    }
}