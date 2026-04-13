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

/* single word(4) 또는 double word(8) alignment */
#define ALIGNMENT 8

/* ALIGNMENT의 가장 가까운 배수로 올림 */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/*
 * mm_init - malloc 패키지를 초기화합니다.
 */
int mm_init(void)
{
    return 0;
}

/*
 * mm_malloc - brk 포인터를 증가시켜 block을 할당합니다.
 *     항상 alignment의 배수 크기를 갖는 block을 할당합니다.
 */
void *mm_malloc(size_t size)
{
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
        return NULL;
    else
    {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
}

/*
 * mm_free - block을 해제해도 아무 일도 하지 않습니다.
 */
void mm_free(void *ptr)
{
}

/*
 * mm_realloc - mm_malloc과 mm_free를 이용해 단순하게 구현합니다.
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