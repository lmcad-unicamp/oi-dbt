/* -*- mode: c -*-
 * $Id: ary3.c 36673 2007-05-03 16:55:46Z laurov $
 * http://www.bagley.org/~doug/shootout/
 *
 * this program is modified from:
 *   http://cm.bell-labs.com/cm/cs/who/bwk/interps/pap.html
 * Timing Trials, or, the Trials of Timing: Experiments with Scripting
 * and User-Interface Languages by Brian W. Kernighan and
 * Christopher J. Van Wyk.
 *
 * I added free() to deallocate memory.
 */
#define LENGTH 5000
int x[LENGTH];
int y[LENGTH];

int main(int argc, char *argv[]) {
  int n = LENGTH;
  int i, k;

  for (i = 0; i < n; i++) 
    x[i] = i + 1;

  for (k=0; k<1000; k++) 
    for (i = n-1; i >= 0; i--) 
      y[i] += x[i];

  return y[n-50];
}
