#ifndef TIMER_HPP
#define TIMER_HPP
#include <string>
#include <ctime>
#include <iostream>

#include <papi.h>

namespace dbt {
  class Timer {
    private:
      int events[5] = {PAPI_L2_TCM, PAPI_TOT_INS, PAPI_TOT_CYC, PAPI_BR_CN, PAPI_BR_MSP}, ret;
      long_long acc[5];
      long_long values[5];

    public:
      Timer() {
        for (int i = 0; i < 5; i++)
          acc[i] = 0;
      }

      void startClock() {
        if ((ret = PAPI_start_counters(events, 5)) != PAPI_OK) {
          fprintf(stderr, "PAPI failed to start counters: %s\n", PAPI_strerror(ret));
          exit(1);
        }
      }

      void stopClock() {
        if ((ret = PAPI_stop_counters(values, 5)) != PAPI_OK) {
          fprintf(stderr, "PAPI failed to read counters: %s\n", PAPI_strerror(ret));
          exit(1);
        }

        for (int i = 0; i < 5; i++)
          acc[i] += values[i];
      }

      void printReport(std::string Title) {
        std::cout << Title << "\n"
          << "Number of Native Instructions Executed: " << (double)acc[1] << "\n"
          << "Total Clock: " << (double)acc[2] << "\n"
          << "Misspredicted Branches: " << (double)acc[4] << "\n"
          << "Total L2 Cache Misses: " << (double)acc[0] << "\n";
      }
  };
}
#endif
