#ifndef DUDECT_H
#define DUDECT_H

#include <vector>
#include <cstdlib>
#include <cstdio>

#define number_percentiles    100
#define number_tests          (1 + number_percentiles + 1 )
#define enough_measurements   10000 // pretty arbitrary
// threshold values for Welch's t-test
#define t_threshold_bananas   15   // test failed, with overwhelming probability
#define t_threshold_moderate  5    // test failed. Pankaj likes 4.5 but let's be more lenient


namespace dut
{
  inline int64_t cpucycles(void) {
   unsigned int hi, lo;
   unsigned int eax = 0;
   unsigned int ebx,ecx,edx;

    asm volatile
    ( "cpuid"
           : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
           : "0"(0)
   );

   __asm__ volatile("rdtsc\n\t" : "=a"(lo), "=d"(hi));
   return ((int64_t)lo) | (((int64_t)hi) << 32);
  }

  typedef struct
  {
    double mean[2] = {0.0, 0.0};
    double m2[2] = {0.0, 0.0};
    double n[2] = {0.0, 0.0};
  } t_ctx;

  void t_push(t_ctx &ctx, double x, uint8_t class_i);
  double t_compute(t_ctx &ctx);


  class Dudect
  {
  private:
    unsigned int number_measurements = 0, measureTimes = 0, totalMeasures = 0;
    int64_t percentiles[number_percentiles];
    int64_t *ticks = NULL, *exec_times = NULL;
    uint8_t* classes = NULL;
    t_ctx *t = NULL;

  public:
    Dudect(int m) {
      number_measurements = m;
      init();
    };

    void init(void)
    {
        ticks       = new int64_t[number_measurements + 1];
        exec_times  = new int64_t[number_measurements];
        classes     = new uint8_t[number_measurements];
        t           = new t_ctx[number_tests];
        for (int i = 0; i<number_percentiles; ++i)
          percentiles[i] = 0;
    }

    void resetData(void)
    {
      //memset(ticks, 0, (number_measurements+1)*sizeof(int64_t));
      //memset(exec_times, 0, number_measurements*sizeof(int64_t));
      //memset(classes, 0, number_measurements*sizeof(uint8_t));
      measureTimes = 0;
    }


    bool measure(int64_t tick, uint8_t classe);
    void doit(void);

    // Fill percentiles. The exponential tendency is mean to approximately match the measurements distribution.
    void prepare_percentiles(void);
    int64_t percentile(double which, size_t length);
    void differentiate(void);
    void wrap_report(t_ctx &x);
    void update_statistics(void);
    int max_test(void);
    void report(void);

    int64_t percentile(int64_t *a, double which, size_t size);
  };
}

#endif /* DUDECT_H */
