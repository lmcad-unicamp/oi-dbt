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
      long_long values[5];
    public:
      void startClock() {
        if ((ret = PAPI_start_counters(events, 5)) != PAPI_OK) {
          fprintf(stderr, "PAPI failed to start counters: %s\n", PAPI_strerror(ret));
          exit(1);
        }
      }

      void stopClock() {
        if ((ret = PAPI_read_counters(values, 5)) != PAPI_OK) {
          fprintf(stderr, "PAPI failed to read counters: %s\n", PAPI_strerror(ret));
          exit(1);
        }
      }

      void printReport(std::string Title, uint64_t num) {
        std::cout << Title << "\n";
        std::cout << "Number of Instructions Emulated: " << num << "\n" 
          << "Number of Native Instructions Executed:" << (double)values[1] << "\n"
          << "Native/Emulated Proportion: " << (double)values[1]/num << "\n"
          << "Total Clock: " << (double)values[2] << "\n"
          << "Clock/Emulated: " << (double)values[2]/num << "\n"
          << "Misspredicted Branches: " << (double)values[4] << "\n"
          << "Total L2 Cache Misses: " << (double)values[0] << "\n";
      }
  };
}
#endif
