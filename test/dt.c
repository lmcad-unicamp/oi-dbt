#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <malloc.h>

enum {
  ITERATIONS = 10,
  size = 20
};

static inline void
double_array_divs_variable(double * __restrict dvec1,
                           double * __restrict dvec2){
  long i, j;
  for(j = 0; j < ITERATIONS; j++)
    for(i = 0; i < size; i++)
      dvec1[i] /= dvec2[i];
  
// Can use loop interchange to turn this into:
//  for(i = 0; i < size; i++) {
//    double tmp1 = dvec1[i];
//    double tmp2 = dvec2[i];
//    for(j = 0; j < ITERATIONS; j++)
//      tmp1 /= tmp2;
//    dvec1[i] = tmp1;
//  }

// Can then hoist 1/tmp2 out of the loop if -ffast-math is enabled.
}

int main(int argc, char *argv[]) {
  double	*dvec1, *dvec2;
  long i;

  dvec1 = malloc(size * sizeof(double));
  dvec2 = malloc(size * sizeof(double));

  //printf( " %i iterations of each test. ", ITERATIONS );
  //printf( " inner loop / array size %i.\n", size );

  // With better alias analysis of posix_memalign, we'd avoid reloading
  // dvec1/dvec2 each time through the loop.
  for( i = 0; i < size; i++ ) {
          printf("%ld = %d\n", size-i, (int)(100*cosf((float) size-i)));
//          dvec1[i] = 0;//1.0  + 0.0000000001 * sinf((float)i);
//          dvec2[i] = 0;//1.0  + 0.0000000001 * sinf((float)i);
  }

  //printf("%d\n", (int)(100*cosf((float) size - i)));
  //double_array_divs_variable( dvec1, dvec2 );
  //printf("%d\n", (int) (10000*dvec1[0]));
  return 1000*dvec1[0];
}

