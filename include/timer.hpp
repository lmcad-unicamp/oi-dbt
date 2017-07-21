#ifndef TIMER_HPP
#define TIMER_HPP
#include <string>
#include <ctime>
#include <sys/time.h>
#include <iostream>

#include <papi.h>

namespace dbt {
  class Timer {
    private:
      #define CounterSize 5
      int events[CounterSize] = {PAPI_L2_TCM, PAPI_TOT_INS, PAPI_TOT_CYC, PAPI_BR_CN, PAPI_BR_MSP}, ret;
      struct timespec start, end;
      long_long acc[CounterSize];
      long_long values[CounterSize];
      long_long base[CounterSize];

    public:
      Timer() {
        if ((ret = PAPI_start_counters(events, CounterSize)) != PAPI_OK) {
          fprintf(stderr, "PAPI failed to start counters: %s\n", PAPI_strerror(ret));
          exit(1);
        }
        if ((ret = PAPI_stop_counters(base, CounterSize)) != PAPI_OK) {
          fprintf(stderr, "PAPI failed to read counters: %s\n", PAPI_strerror(ret));
          exit(1);
        }

        for (int i = 0; i < CounterSize; i++)
          acc[i] = 0;
      }

      void startClock() {
        clock_gettime(CLOCK_MONOTONIC_RAW, &start);
        if ((ret = PAPI_start_counters(events, CounterSize)) != PAPI_OK) {
          fprintf(stderr, "PAPI failed to start counters: %s\n", PAPI_strerror(ret));
          exit(1);
        }
      }

      void stopClock() {
        if ((ret = PAPI_stop_counters(values, CounterSize)) != PAPI_OK) {
          fprintf(stderr, "PAPI failed to read counters: %s\n", PAPI_strerror(ret));
          exit(1);
        }
        clock_gettime(CLOCK_MONOTONIC_RAW, &end);

        for (int i = 0; i < CounterSize; i++)
          acc[i] += values[i];
      }

      void printReport(std::string Title) {
        uint64_t delta_us = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
        std::cerr << Title << "\n"
          << "Number of Native Instructions Executed: " << (double) (acc[1] - base[1]) << "\n"
          << "Total Clock: " << (double) (acc[2] - base[2]) << "\n"
          << "Misspredicted Branches: " << (double) (acc[4] - base[3]) << "\n"
          << "Total L2 Cache Misses: " << (double) (acc[0] - base[0]) << "\n"
          << "Time (s): " << (double) (delta_us/1000000.0) << "\n";
      }
  };
}
#endif
