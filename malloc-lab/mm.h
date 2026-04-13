#include <stdio.h>

extern int mm_init (void);
extern void *mm_malloc (size_t size);
extern void mm_free (void *ptr);
extern void *mm_realloc(void *ptr, size_t size);


/* 
 * 학생은 1명 또는 2명 팀으로 작업합니다. 팀은 이 타입의 struct에
 * 팀 이름, 개인 이름, login ID를 bits.c 파일에 입력합니다.
 */
typedef struct {
    char *teamname; /* ID1+ID2 또는 ID1 */
    char *name1;    /* 첫 번째 팀원의 전체 이름 */
    char *id1;      /* 첫 번째 팀원의 login ID */
    char *name2;    /* 두 번째 팀원의 전체 이름(있는 경우) */
    char *id2;      /* 두 번째 팀원의 login ID */
} team_t;

extern team_t team;
