#ifndef __CONFIG_H_
#define __CONFIG_H_

/*
 * config.h - malloc lab 설정 파일
 *
 * Copyright (c) 2002, R. Bryant and D. O'Hallaron, 모든 권리 보유.
 * 허가 없이 사용, 수정, 복사할 수 없습니다.
 */

/*
 * driver가 기본 tracefiles를 찾을 때 사용할 기본 경로입니다.
 * 실행 시 -t 플래그로 덮어쓸 수 있습니다.
 */
#define TRACEDIR "./traces/"

/*
 * driver가 테스트에 사용할 TRACEDIR 내 기본 tracefiles 목록입니다.
 * driver의 테스트 세트에서 trace를 추가하거나 제거하고 싶다면
 * 이 목록을 수정하세요. 예를 들어 학생들에게 realloc 구현을
 * 요구하지 않으려면 마지막 두 trace를 삭제하면 됩니다.
 */
#define DEFAULT_TRACEFILES \
  "amptjp-bal.rep",\
  "cccp-bal.rep",\
  "cp-decl-bal.rep",\
  "expr-bal.rep",\
  "coalescing-bal.rep",\
  "random-bal.rep",\
  "random2-bal.rep",\
  "binary-bal.rep",\
  "binary2-bal.rep",\
  "realloc-bal.rep",\
  "realloc2-bal.rep"

/*
 * 이 상수는 기준 시스템 하나에서 우리 trace를 사용해 측정한
 * libc malloc 패키지의 추정 성능을 나타냅니다. 보통 학생들이
 * 사용하는 것과 같은 종류의 시스템을 가정합니다. 목적은
 * throughput이 성능 지수에 기여하는 비중에 상한을 두는 것입니다.
 * 학생들이 AVG_LIBC_THRUPUT를 넘어서면 점수상 추가 이득은 없습니다.
 * 이는 매우 빠르지만 지나치게 단순한 malloc 패키지를 만드는 일을
 * 억제하기 위한 장치입니다.
 */
#define AVG_LIBC_THRUPUT      600E3  /* 600 Kops/sec */

 /* 
  * 이 상수는 space utilization(UTIL_WEIGHT)과 throughput
  * (1 - UTIL_WEIGHT)이 성능 지수에 얼마나 기여할지 결정합니다.
  */
#define UTIL_WEIGHT .60

/* 
 * 바이트 단위 alignment 요구사항(4 또는 8)
 */
#define ALIGNMENT 8  

/* 
 * 바이트 단위 최대 heap 크기
 */
#define MAX_HEAP (20*(1<<20))  /* 20 MB */

/*****************************************************************************
 * timing method를 선택하려면 아래 USE_xxx 상수 중 정확히 하나만 "1"로 설정하세요.
 *****************************************************************************/
#define USE_FCYC   0   /* K-best scheme을 사용하는 cycle counter(x86와 Alpha만) */
#define USE_ITIMER 0   /* interval timer(모든 Unix 계열) */
#define USE_GETTOD 1   /* gettimeofday(모든 Unix 계열) */

#endif /* __CONFIG_H */
