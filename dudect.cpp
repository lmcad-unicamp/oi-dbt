#include <manager.hpp>
#include <OIPrinter.hpp>
#include <fstream>
#include <vector>
#include <dudect.hpp>
#include <iostream>
#include <ostream>
#include <cmath>
#include <cstdio>
#include <assert.h>
#include <cstdlib>

using namespace dut;

extern std::ofstream DudectFile;

void dut::t_push(t_ctx &ctx, double x, uint8_t class_i) {
  assert(class_i == 0 || class_i == 1);
  ctx.n[class_i]++;
  // Welford method for computing online variance in a numerically stable way.
  double delta = x - ctx.mean[class_i];
  ctx.mean[class_i] = ctx.mean[class_i] + delta / ctx.n[class_i];
  ctx.m2[class_i] = ctx.m2[class_i] + delta * (x - ctx.mean[class_i]);
}


double dut::t_compute(t_ctx &ctx) {
  double var[2] = {0.0, 0.0};
  var[0] = ctx.m2[0] / (ctx.n[0] - 1);
  var[1] = ctx.m2[1] / (ctx.n[1] - 1);
  double num = (ctx.mean[0] - ctx.mean[1]);
  double den = sqrt(var[0] / ctx.n[0] + var[1] / ctx.n[1]);
  double t_value = num / den;
  return t_value;
}





static int cmp(const int64_t *a, const int64_t *b) { return (int)(*a - *b); }

void Dudect::prepare_percentiles(void) {
  for (size_t i = 0; i < number_percentiles; i++) {
    percentiles[i] = percentile(1 - (pow(0.5, 10 * (double)(i + 1) / number_percentiles)), number_percentiles);
  }
}

int64_t Dudect::percentile(double which, size_t size)
{
  qsort(percentiles, size, sizeof(int64_t), (int (*)(const void *, const void *))cmp);
  size_t array_position = (size_t)((double)size * (double)which);
  assert(array_position >= 0);
  assert(array_position < size);
  //printf("percentile %f is %lld\n", which, a[array_position]);
  return percentiles[array_position];
}





bool Dudect::measure(int64_t tick, uint8_t classe)
{
  //std::cout << "[" << measureTimes << "]: " << tick << std::endl;
  totalMeasures++;
  static int64_t LastTick = 0;

  if(measureTimes >= number_measurements)
    return false;

  if (tick < 1000000)
    tick = LastTick;

  //if (totalMeasures%100 == 0)
    //std::cout << "[" << totalMeasures << "]: " << tick << "\n";

  DudectFile << "[" << totalMeasures << "]: " << tick << "\n";

  classes[measureTimes] = classe;
  ticks[measureTimes] = tick;
  measureTimes++;
  LastTick = tick;

  return true;
}

void Dudect::differentiate(void)
{
  for (size_t i = 0; i < number_measurements; i++) {
    exec_times[i] = ticks[i + 1] - ticks[i];
  }
}


void Dudect::update_statistics(void)
{
  // XXX we could throw away the first 1e4 and the last 1e4 measurements,
  // to minimize measurement noise. test if this actually improves anything.
  for (int i = number_measurements/10; i < number_measurements-number_measurements/10; i++) {
    int64_t difference = exec_times[i];

    if (difference < 0)
      continue; // the cpu cycle counter overflowed

    // do a t-test on the execution time
    t_push(t[0], difference, classes[i]);

    // do a t-test on cropped execution times, for several cropping thresholds.
    for (size_t crop_index = 0; crop_index < number_percentiles; crop_index++) {
      if (difference < percentiles[crop_index]) {
        t_push(t[crop_index + 1], difference, classes[i]);
      }
    }

    // do a second-order test (only if we have more than 10000 measurements).
    // Centered product pre-processing.
    if (t[0].n[0] > 10000) {
      double centered = (double)difference - t[0].mean[classes[i]];
      t_push(t[1 + number_percentiles], centered * centered, classes[i]);
    }
  }
}


void Dudect::wrap_report(t_ctx &x) {
  if (x.n[0] > enough_measurements) {
    double tval = t_compute(x);
    std::cout << "Got t=" << tval << "\n";
    DudectFile << "Got t=" << tval << "\n";
  } else {
    std::cout << " (not enough measurements" << x.n[0] << ")\n";
    DudectFile << " (not enough measurements" << x.n[0] << ")\n";
  }
}

// which t-test yields max t value?
int Dudect::max_test(void) {
  int ret = 0;
  double max = 0;
  for (size_t i = 0; i < number_tests; i++) {
    if (t[i].n[0] > enough_measurements) {
      double x = fabs(t_compute(t[i]));
      if (max < x) {
        max = x;
        ret = i;
      }
    }
  }
  return ret;
}

void Dudect::report(void) {
  static bool started = false;

  #if 0
  printf("number traces\n");
  for (size_t i = 0; i < number_tests; i++) {
    printf("traces %zu %f\n", i, t[i].n[0] +  t[i].n[1]);
  }
  printf("\n\n");

  printf("first order\n");
  wrap_report(t[0]);

  printf("cropped\n");
  for (size_t i = 0; i < number_percentiles; i++) {
    wrap_report(t[i + 1]);
  }

  printf("second order\n");
  wrap_report(t[1 + number_percentiles]);
  #endif

  int mt = max_test();
  double max_t = fabs(t_compute(t[mt]));
  double number_traces_max_t = t[mt].n[0] +  t[mt].n[1];
  double max_tau = max_t / sqrt(number_traces_max_t);

  std::cout << "meas " << (number_traces_max_t / 1e6) << "M, ";
  DudectFile << "meas " << (number_traces_max_t / 1e6) << "M, ";
  if (number_traces_max_t < enough_measurements) {
    std::cout << "not enough measurements " << enough_measurements-number_traces_max_t << " still to go).\n";

    DudectFile << "not enough measurements " << enough_measurements-number_traces_max_t << " still to go).\n";
    return;
  }

  if (!started)
  {
    std::cout << "Number of measurements to start: " << totalMeasures << "\n";
    DudectFile << "Number of measurements to start: " << totalMeasures << "\n";
  }


  started = true;

  /*
   * max_t: the t statistic value
   * max_tau: a t value normalized by sqrt(number of measurements).
   *          this way we can compare max_tau taken with different
   *          number of measurements. This is sort of "distance
   *          between distributions", independent of number of
   *          measurements.
   * (5/tau)^2: how many measurements we would need to barely
   *            detect the leak, if present. "barely detect the
   *            leak" = have a t value greater than 5.
   */

  std::cout << "Max t: +" << max_t << ", Max tau: " << max_tau << ", (5/tau)^2: " << (double)(5*5)/(double)(max_tau*max_tau);
  DudectFile << "Max t: +" << max_t << ", Max tau: " << max_tau << ", (5/tau)^2: " << (double)(5*5)/(double)(max_tau*max_tau);

  if (max_t > t_threshold_bananas) {
    std::cout << " -- Definitely not constant time!\n";
    DudectFile << " -- Definitely not constant time!\n";
    return;
    //exit(0);
  }
  if (max_t > t_threshold_moderate) {
    std::cout << " -- Probably not constant time...\n";
    DudectFile << " -- Probably not constant time...\n";
    return;
  }
  if (max_t < t_threshold_moderate) {
    std::cout << " -- For the moment, maybe constant time.\n";
    DudectFile << " -- For the moment, maybe constant time.\n";
    return;
  }
}

void Dudect::doit(void)
{
  differentiate();

  if(percentiles[number_percentiles-1] == 0)
    prepare_percentiles();

  update_statistics();
  report();

  resetData();
}

/*
static void doit(void) {
  // XXX move these callocs to parent
  int64_t *ticks = calloc(number_measurements + 1, sizeof(int64_t));
  int64_t *exec_times = calloc(number_measurements, sizeof(int64_t));
  uint8_t *classes = calloc(number_measurements, sizeof(uint8_t));
  uint8_t *input_data =
      calloc(number_measurements * chunk_size, sizeof(uint8_t));

  if (!ticks || !exec_times || !classes || !input_data) {
    die();
  }

  prepare_inputs(input_data, classes);
  measure(ticks, input_data);
  differentiate(exec_times, ticks); // inplace

  // we compute the percentiles only if they are not filled yet
  if (percentiles[number_percentiles - 1] == 0) {
    prepare_percentiles(exec_times);
  }
  update_statistics(exec_times, classes);
  report();

  free(ticks);
  free(exec_times);
  free(classes);
  free(input_data);
}*/

/*
int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  init_dut();
  for (int i = 0; i < number_tests; i++) {
    t[i] = malloc(sizeof(t_ctx));
    t_init(t[i]);
  }

  for (;;) {
    doit();
  }
}
*/
