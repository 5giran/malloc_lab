/*
 * fcyc.c - 함수 f가 사용하는 시간을 CPU cycle 단위로 추정합니다.
 * 
 * Copyright (c) 2002, R. Bryant and D. O'Hallaron, 모든 권리 보유.
 * 허가 없이 사용, 수정, 복사할 수 없습니다.
 *
 * clock.c의 cycle timer 루틴을 사용해 함수 f의 실행 시간을
 * CPU cycle 단위로 추정합니다.
 */
#include <stdlib.h>
#include <sys/times.h>
#include <stdio.h>

#include "fcyc.h"
#include "clock.h"

/* 기본값 */
#define K 3                  /* K-best scheme에서 사용할 K 값 */
#define MAXSAMPLES 20        /* MAXSAMPLES 이후에는 시도를 중단 */
#define EPSILON 0.01         /* K개 샘플이 서로 EPSILON 이내여야 함 */
#define COMPENSATE 0         /* 1이면 clock tick 보정을 시도 */
#define CLEAR_CACHE 0        /* 테스트 함수 실행 전에 cache를 비움 */
#define CACHE_BYTES (1<<19)  /* 최대 cache 크기(바이트) */
#define CACHE_BLOCK 32       /* cache block 크기(바이트) */

static int kbest = K;
static int maxsamples = MAXSAMPLES;
static double epsilon = EPSILON;
static int compensate = COMPENSATE;
static int clear_cache = CLEAR_CACHE;
static int cache_bytes = CACHE_BYTES;
static int cache_block = CACHE_BLOCK;

static int *cache_buf = NULL;

static double *values = NULL;
static int samplecount = 0;

/* 디버깅 전용 */
#define KEEP_VALS 0
#define KEEP_SAMPLES 0

#if KEEP_SAMPLES
static double *samples = NULL;
#endif

/* 
 * init_sampler - 새 sampling 과정을 시작합니다.
 */
static void init_sampler()
{
    if (values)
	free(values);
    values = calloc(kbest, sizeof(double));
#if KEEP_SAMPLES
    if (samples)
	free(samples);
    /* wraparound 분석을 위해 여유 공간을 추가로 할당 */
    samples = calloc(maxsamples+kbest, sizeof(double));
#endif
    samplecount = 0;
}

/* 
 * add_sample - 새 sample을 추가합니다.
 */
static void add_sample(double val)
{
    int pos = 0;
    if (samplecount < kbest) {
	pos = samplecount;
	values[pos] = val;
    } else if (val < values[kbest-1]) {
	pos = kbest-1;
	values[pos] = val;
    }
#if KEEP_SAMPLES
    samples[samplecount] = val;
#endif
    samplecount++;
    /* 삽입 정렬 */
    while (pos > 0 && values[pos-1] > values[pos]) {
	double temp = values[pos-1];
	values[pos-1] = values[pos];
	values[pos] = temp;
	pos--;
    }
}

/* 
 * has_converged - kbest 최소 측정값이 epsilon 범위 안으로 수렴했는가?
 */
static int has_converged()
{
    return
	(samplecount >= kbest) &&
	((1 + epsilon)*values[0] >= values[kbest-1]);
}

/* 
 * clear - cache를 비우는 코드
 */
static volatile int sink = 0;

static void clear()
{
    int x = sink;
    int *cptr, *cend;
    int incr = cache_block/sizeof(int);
    if (!cache_buf) {
	cache_buf = malloc(cache_bytes);
	if (!cache_buf) {
	    fprintf(stderr, "Fatal error.  Malloc returned null when trying to clear cache\n");
	    exit(1);
	}
    }
    cptr = (int *) cache_buf;
    cend = cptr + cache_bytes/sizeof(int);
    while (cptr < cend) {
	x += *cptr;
	cptr += incr;
    }
    sink = x;
}

/*
 * fcyc - K-best scheme을 사용해 함수 f의 실행 시간을 추정합니다.
 */
double fcyc(test_funct f, void *argp)
{
    double result;
    init_sampler();
    if (compensate) {
	do {
	    double cyc;
	    if (clear_cache)
		clear();
	    start_comp_counter();
	    f(argp);
	    cyc = get_comp_counter();
	    add_sample(cyc);
	} while (!has_converged() && samplecount < maxsamples);
    } else {
	do {
	    double cyc;
	    if (clear_cache)
		clear();
	    start_counter();
	    f(argp);
	    cyc = get_counter();
	    add_sample(cyc);
	} while (!has_converged() && samplecount < maxsamples);
    }
#ifdef DEBUG
    {
	int i;
	printf(" %d smallest values: [", kbest);
	for (i = 0; i < kbest; i++)
	    printf("%.0f%s", values[i], i==kbest-1 ? "]\n" : ", ");
    }
#endif
    result = values[0];
#if !KEEP_VALS
    free(values); 
    values = NULL;
#endif
    return result;  
}


/*************************************************************
 * 측정 루틴에서 사용하는 여러 파라미터를 설정합니다.
 ************************************************************/

/* 
 * set_fcyc_clear_cache - 설정하면 각 측정 전에 cache를 비우는
 *     코드를 실행합니다.
 *     기본값 = 0
 */
void set_fcyc_clear_cache(int clear)
{
    clear_cache = clear;
}

/* 
 * set_fcyc_cache_size - cache를 비울 때 사용할 cache 크기를 설정합니다.
 *     기본값 = 1<<19 (512KB)
 */
void set_fcyc_cache_size(int bytes)
{
    if (bytes != cache_bytes) {
	cache_bytes = bytes;
	if (cache_buf) {
	    free(cache_buf);
	    cache_buf = NULL;
	}
    }
}

/* 
 * set_fcyc_cache_block - cache block 크기를 설정합니다.
 *     기본값 = 32
 */
void set_fcyc_cache_block(int bytes) {
    cache_block = bytes;
}


/* 
 * set_fcyc_compensate - 설정하면 timer interrupt overhead를
 *     보정하려고 시도합니다.
 *     기본값 = 0
 */
void set_fcyc_compensate(int compensate_arg)
{
    compensate = compensate_arg;
}

/* 
 * set_fcyc_k - K-best 측정 scheme에서 사용할 K 값
 *     기본값 = 3
 */
void set_fcyc_k(int k)
{
    kbest = k;
}

/* 
 * set_fcyc_maxsamples - 어떤 허용 오차 안에서 K-best를 찾기 위해
 *     시도할 최대 sample 수입니다.
 *     이를 넘기면 지금까지 찾은 최적 sample을 반환합니다.
 *     기본값 = 20
 */
void set_fcyc_maxsamples(int maxsamples_arg)
{
    maxsamples = maxsamples_arg;
}

/* 
 * set_fcyc_epsilon - K-best에 필요한 허용 오차
 *     기본값 = 0.01
 */
void set_fcyc_epsilon(double epsilon_arg)
{
    epsilon = epsilon_arg;
}




