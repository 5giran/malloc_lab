/* 
 * clock.c - x86, Alpha, Sparc 시스템에서 cycle counter를
 *           사용하기 위한 루틴입니다.
 * 
 * Copyright (c) 2002, R. Bryant and D. O'Hallaron, 모든 권리 보유.
 * 허가 없이 사용, 수정, 복사할 수 없습니다.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/times.h>
#include "clock.h"


/******************************************************* 
 * 머신 의존 함수
 *
 * 참고: __i386__와 __alpha 상수는
 * GCC가 C 전처리기를 호출할 때 설정합니다.
 * gcc -v로 직접 확인할 수 있습니다.
 *******************************************************/

#if defined(__i386__)  
/*******************************************************
 * Pentium용 start_counter()와 get_counter() 버전
 *******************************************************/


/* $begin x86cyclecounter */
/* cycle counter를 초기화 */
static unsigned cyc_hi = 0;
static unsigned cyc_lo = 0;


/* *hi와 *lo를 cycle counter의 상위/하위 비트로 설정합니다.
   구현에는 rdtsc 명령을 사용하는 assembly 코드가 필요합니다. */
void access_counter(unsigned *hi, unsigned *lo)
{
    asm("rdtsc; movl %%edx,%0; movl %%eax,%1"   /* cycle counter를 읽고 */
	: "=r" (*hi), "=r" (*lo)                /* 결과를 두 출력값으로 */
	: /* 입력 없음 */                        /* 옮깁니다 */
	: "%edx", "%eax");
}

/* cycle counter의 현재 값을 기록합니다. */
void start_counter()
{
    access_counter(&cyc_hi, &cyc_lo);
}

/* 마지막 start_counter 호출 이후의 cycle 수를 반환합니다. */
double get_counter()
{
    unsigned ncyc_hi, ncyc_lo;
    unsigned hi, lo, borrow;
    double result;

    /* cycle counter를 읽음 */
    access_counter(&ncyc_hi, &ncyc_lo);

    /* double precision 뺄셈 수행 */
    lo = ncyc_lo - cyc_lo;
    borrow = lo > ncyc_lo;
    hi = ncyc_hi - cyc_hi - borrow;
    result = (double) hi * (1 << 30) * 4 + lo;
    if (result < 0) {
	fprintf(stderr, "Error: counter returns neg value: %.0f\n", result);
    }
    return result;
}
/* $end x86cyclecounter */

#elif defined(__alpha)

/****************************************************
 * Alpha용 start_counter()와 get_counter() 버전
 ***************************************************/

/* cycle counter를 초기화 */
static unsigned cyc_hi = 0;
static unsigned cyc_lo = 0;


/* Alpha cycle timer를 사용해 cycle을 계산한 뒤
   측정된 clock speed로 초 단위 시간을 계산합니다.
*/

/*
 * counterRoutine은 Alpha의 processor cycle counter에 접근하는
 * Alpha 명령어 배열입니다. counter에 접근하기 위해 rpcc 명령을
 * 사용합니다. 이 64비트 레지스터는 두 부분으로 나뉩니다.
 * 하위 32비트는 현재 프로세스가 사용한 cycle 수이고, 상위 32비트는
 * wall clock cycle입니다. 이 명령어들은 counter를 읽은 뒤
 * 하위 32비트를 unsigned int로 변환합니다. 이것이 user space의
 * counter 값입니다.
 * 참고: 이 counter는 측정 가능한 시간이 매우 짧습니다.
 * 450MHz clock에서는 약 9초 정도만 측정할 수 있습니다. */
static unsigned int counterRoutine[] =
{
    0x601fc000u,
    0x401f0000u,
    0x6bfa8001u
};

/* 위 명령어 배열을 함수로 캐스팅합니다. */
static unsigned int (*counter)(void)= (void *)counterRoutine;


void start_counter()
{
    /* cycle counter를 읽음 */
    cyc_hi = 0;
    cyc_lo = counter();
}

double get_counter()
{
    unsigned ncyc_hi, ncyc_lo;
    unsigned hi, lo, borrow;
    double result;
    ncyc_lo = counter();
    ncyc_hi = 0;
    lo = ncyc_lo - cyc_lo;
    borrow = lo > ncyc_lo;
    hi = ncyc_hi - cyc_hi - borrow;
    result = (double) hi * (1 << 30) * 4 + lo;
    if (result < 0) {
	fprintf(stderr, "Error: Cycle counter returning negative value: %.0f\n", result);
    }
    return result;
}

#else

/****************************************************************
 * 아래는 cycle counter 루틴을 아직 구현하지 않은 다른 플랫폼입니다.
 * 더 최신 Sparc(v8plus) 모델에는 사용자 프로그램에서 접근 가능한
 * cycle counter가 있지만, 이를 지원하지 않는 Sparc 시스템도 아직 많아
 * 여기서는 Sparc 버전을 제공하지 않습니다.
 ***************************************************************/

void start_counter()
{
    printf("ERROR: You are trying to use a start_counter routine in clock.c\n");
    printf("that has not been implemented yet on this platform.\n");
    printf("Please choose another timing package in config.h.\n");
    exit(1);
}

double get_counter() 
{
    printf("ERROR: You are trying to use a get_counter routine in clock.c\n");
    printf("that has not been implemented yet on this platform.\n");
    printf("Please choose another timing package in config.h.\n");
    exit(1);
}
#endif




/*******************************
 * 머신 독립 함수
 ******************************/
double ovhd()
{
    /* cache 효과를 제거하기 위해 두 번 수행 */
    int i;
    double result;

    for (i = 0; i < 2; i++) {
	start_counter();
	result = get_counter();
    }
    return result;
}

/* $begin mhz */
/* sleeptime초 동안 sleep하는 동안 경과한 cycle 수를 측정해 */
/* clock rate를 추정합니다 */
double mhz_full(int verbose, int sleeptime)
{
    double rate;

    start_counter();
    sleep(sleeptime);
    rate = get_counter() / (1e6*sleeptime);
    if (verbose) 
	printf("Processor clock rate ~= %.1f MHz\n", rate);
    return rate;
}
/* $end mhz */

/* 기본 sleeptime을 사용하는 버전 */
double mhz(int verbose)
{
    return mhz_full(verbose, 2);
}

/** timer interrupt overhead를 보정하는 특수 counter */

static double cyc_per_tick = 0.0;

#define NEVENT 100
#define THRESHOLD 1000
#define RECORDTHRESH 3000

/* timer interrupt가 얼마나 시간을 쓰는지 추정 */
static void callibrate(int verbose)
{
    double oldt;
    struct tms t;
    clock_t oldc;
    int e = 0;

    times(&t);
    oldc = t.tms_utime;
    start_counter();
    oldt = get_counter();
    while (e <NEVENT) {
	double newt = get_counter();

	if (newt-oldt >= THRESHOLD) {
	    clock_t newc;
	    times(&t);
	    newc = t.tms_utime;
	    if (newc > oldc) {
		double cpt = (newt-oldt)/(newc-oldc);
		if ((cyc_per_tick == 0.0 || cyc_per_tick > cpt) && cpt > RECORDTHRESH)
		    cyc_per_tick = cpt;
		/*
		  if (verbose)
		  printf("%.0f cycle과 %d tick이 걸린 이벤트를 관측. 비율 = %f\n",
		  newt-oldt, (int) (newc-oldc), cpt);
		*/
		e++;
		oldc = newc;
	    }
	    oldt = newt;
	}
    }
    if (verbose)
	printf("Setting cyc_per_tick to %f\n", cyc_per_tick);
}

static clock_t start_tick = 0;

void start_comp_counter() 
{
    struct tms t;

    if (cyc_per_tick == 0.0)
	callibrate(0);
    times(&t);
    start_tick = t.tms_utime;
    start_counter();
}

double get_comp_counter() 
{
    double time = get_counter();
    double ctime;
    struct tms t;
    clock_t ticks;

    times(&t);
    ticks = t.tms_utime - start_tick;
    ctime = time - ticks*cyc_per_tick;
    /*
      printf("측정된 cycle %.0f. Ticks = %d. 보정 후 %.0f cycles\n",
      time, (int) ticks, ctime);
    */
    return ctime;
}
