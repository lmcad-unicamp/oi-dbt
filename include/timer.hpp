#ifndef TIMER_HPP
#define TIMER_HPP
#include <string>
#include <ctime>
#include <sys/time.h>
#include <iostream>
#include <fstream>

#include <papi.h>

namespace dbt {
  class Timer {
    private:
      #define CounterSize 5
    //PAPI_L2_TCM
//      int events[CounterSize] = {PAPI_TOT_INS, PAPI_TOT_CYC, PAPI_BR_CN, PAPI_BR_MSP, PAPI_L1_ICM}, ret;
      struct timespec start, end;
//      long_long acc[CounterSize];
//      long_long values[CounterSize];
//      long_long base[CounterSize];

    public:
      Timer() {
/*        if ((ret = PAPI_start_counters(events, CounterSize)) != PAPI_OK) {
          fprintf(stderr, "PAPI failed to start counters: %s\n", PAPI_strerror(ret));
          exit(1);
        }
        if ((ret = PAPI_stop_counters(base, CounterSize)) != PAPI_OK) {
          fprintf(stderr, "PAPI failed to read counters: %s\n", PAPI_strerror(ret));
          exit(1);
        }

        for (int i = 0; i < CounterSize; i++)
          acc[i] = 0;*/
      }

      void startClock() {
        clock_gettime(CLOCK_MONOTONIC_RAW, &start);
        /*
        if ((ret = PAPI_start_counters(events, CounterSize)) != PAPI_OK) {
          fprintf(stderr, "PAPI failed to start counters: %s\n", PAPI_strerror(ret));
          exit(1);
        }*/
      }

      void stopClock() {
/*        if ((ret = PAPI_stop_counters(values, CounterSize)) != PAPI_OK) {
          fprintf(stderr, "PAPI failed to read counters: %s\n", PAPI_strerror(ret));
          exit(1);
        }*/
        clock_gettime(CLOCK_MONOTONIC_RAW, &end);
/*
        for (int i = 0; i < CounterSize; i++)
          acc[i] += values[i];*/
      }

      void printReport(std::string Title) {
        uint64_t delta_us = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
  
        std::cerr << Title << std::endl
/*          << "Number of Native Instructions Executed: " << (double) (acc[0] - base[0]) << std::endl
          << "Total Cycles: " << (double) (acc[1] - base[1]) << std::endl
          << "Conditional branch instructions executed: " << (double) (acc[2] - base[2]) << std::endl
          << "Conditional branch instructions misses: " << (double) (acc[3] - base[3]) << std::endl
          << "Total L1 I-Cache misses: " << (double) (acc[4] - base[4]) << std::endl*/
          << "Time (s): " << (double) (delta_us/1000000.0) << std::endl;
      }
      
      void printReport(std::string Title, std::ofstream& reportFile)
      {
        uint64_t delta_us = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
        
/*        reportFile << (double) (acc[0] - base[0]) <<    std::endl;    //Native exec
        reportFile << (double) (acc[1] - base[1]) <<    std::endl;    //Total Cycles
        reportFile << (double) (acc[2] - base[2]) <<    std::endl;    //Condiitonal branch instructions executed
        reportFile << (double) (acc[3] - base[3]) <<    std::endl;    //Conditional branch instructions misses
        reportFile << (double) (acc[4] - base[4]) <<    std::endl;    //Total L1 I-Cache Misses*/
        reportFile << (double) (delta_us/1000000.0) <<  std::endl;    //Time (s)
      }


  };
}
#endif
