/*
 * memlib.c - 메모리 시스템을 시뮬레이션하는 모듈입니다.
 *            학생의 malloc 패키지 호출과 libc의 시스템 malloc 패키지 호출을
 *            번갈아 실행할 수 있게 해 주므로 필요합니다.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>

#include "memlib.h"
#include "config.h"

/* 내부 변수 */
static char *mem_start_brk;  /* heap의 첫 번째 바이트를 가리킴 */
static char *mem_brk;        /* heap의 마지막 바이트를 가리킴 */
static char *mem_max_addr;   /* 합법적인 heap 주소의 최댓값 */

/* 
 * mem_init - 메모리 시스템 모델을 초기화합니다.
 */
void mem_init(void)
{
    /* 사용 가능한 VM을 모델링할 저장 공간을 할당합니다 */
    if ((mem_start_brk = (char *)malloc(MAX_HEAP)) == NULL) {
	fprintf(stderr, "mem_init_vm: malloc error\n");
	exit(1);
    }

    mem_max_addr = mem_start_brk + MAX_HEAP;  /* 합법적인 heap 주소의 최댓값 */
    mem_brk = mem_start_brk;                  /* 처음에는 heap이 비어 있음 */
}

/* 
 * mem_deinit - 메모리 시스템 모델이 사용한 저장 공간을 해제합니다.
 */
void mem_deinit(void)
{
    free(mem_start_brk);
}

/*
 * mem_reset_brk - 시뮬레이션된 brk 포인터를 초기화해 빈 heap으로 만듭니다.
 */
void mem_reset_brk()
{
    mem_brk = mem_start_brk;
}

/* 
 * mem_sbrk - sbrk 함수의 단순 모델입니다. heap을 incr 바이트만큼
 *    확장하고 새 영역의 시작 주소를 반환합니다. 이 모델에서는
 *    heap을 줄일 수 없습니다.
 */
void *mem_sbrk(int incr) 
{
    char *old_brk = mem_brk;

    if ( (incr < 0) || ((mem_brk + incr) > mem_max_addr)) {
	errno = ENOMEM;
	fprintf(stderr, "ERROR: mem_sbrk failed. Ran out of memory...\n");
	return (void *)-1; // 실제 유효한 주소는 절대 이 값이 될수 없음, 잘못됐다는 뜻
    }
    mem_brk += incr;
    return (void *)old_brk;
}

/*
 * mem_heap_lo - heap의 첫 번째 바이트 주소를 반환합니다.
 */
void *mem_heap_lo()
{
    return (void *)mem_start_brk;
}

/* 
 * mem_heap_hi - heap의 마지막 바이트 주소를 반환합니다.
 */
void *mem_heap_hi()
{
    return (void *)(mem_brk - 1);
}

/*
 * mem_heapsize() - heap 크기를 바이트 단위로 반환합니다.
 */
size_t mem_heapsize() 
{
    return (size_t)(mem_brk - mem_start_brk);
}

/*
 * mem_pagesize() - 시스템의 page 크기를 반환합니다.
 */
size_t mem_pagesize()
{
    return (size_t)getpagesize();
}
