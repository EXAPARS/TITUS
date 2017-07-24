#ifndef __RDTSC_H_
#define __RDTSC_H_


// define time primitives
#ifndef TITUS_PROC_FREQ
#define TITUS_PROC_FREQ          2.6e9
#endif


static inline uint64_t rdtsc()
{
	uint64_t a, d;
	__asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
	return (d<<32) | a;
}
typedef uint64_t TITUS_Logger_raw_time_t;

#endif // __RDTSC_H_
