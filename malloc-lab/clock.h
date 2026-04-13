/* cycle counter를 사용하는 루틴 */

/* counter를 시작 */
void start_counter();

/* counter 시작 이후의 cycle 수를 가져옴 */
double get_counter();

/* counter overhead를 측정 */
double ovhd();

/* processor의 clock rate를 결정(기본 sleeptime 사용) */
double mhz(int verbose);

/* 정확도를 더 세밀하게 제어하며 processor의 clock rate를 결정 */
double mhz_full(int verbose, int sleeptime);

/** timer interrupt overhead를 보정하는 특수 counter */

void start_comp_counter();

double get_comp_counter();
