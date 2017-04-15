/* -*- mode: c -*-
 * $Id: sieve.c 36673 2007-05-03 16:55:46Z laurov $
 * http://www.bagley.org/~doug/shootout/
 */

int
main(int argc, char *argv[]) {
#define LENGTH 1700
  int NUM = LENGTH;
  static char flags[8192 + 1];
  long i, k;
  int count = 0;

  while (NUM--) {
    count = 0; 
    for (i=2; i <= 8192; i++) {
      flags[i] = 1;
    }
    for (i=2; i <= 8192; i++) {
      if (flags[i]) {
        for (k=i+i; k <= 8192; k+=i) {
          flags[k] = 0;
        }
        count++;
      }
    }
  }
  return(count);
}

