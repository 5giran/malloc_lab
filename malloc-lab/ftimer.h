/* 
 * 함수용 timer
 */
typedef void (*ftimer_test_funct)(void *); 

/* Unix interval timer를 사용해 f(argp)의 실행 시간을 추정합니다.
   n번 실행한 평균을 반환합니다 */
double ftimer_itimer(ftimer_test_funct f, void *argp, int n);


/* gettimeofday를 사용해 f(argp)의 실행 시간을 추정합니다.
   n번 실행한 평균을 반환합니다 */
double ftimer_gettod(ftimer_test_funct f, void *argp, int n);
