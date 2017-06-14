#define IM 21
#define IA 12
#define IC 13

unsigned int gen_random();
void heapsort(int n, unsigned int *ra);

int
main(int argc, char *argv[]) {
  int N = 300;
  unsigned int ary[301];
  int i;
    
  for (i=1; i<=N; i++) {
    ary[i] = gen_random();
  }

  heapsort(N, ary);

  return ary[10] + ary[N-1];
}

unsigned int
gen_random() {
  static unsigned int last = 42;
  return( last = (last + IA - IC)); //% IM);
}

void
heapsort(int n, unsigned int *ra) {
  int i, j;
  int ir = n;
  int l = (n >> 1) + 1;
  unsigned int rra;

  for (;;) {
    if (l > 1) {
      rra = ra[--l];
    } else {
      rra = ra[ir];
      ra[ir] = ra[1];
      if (--ir == 1) {
        ra[1] = rra;
        return;
      }
    }
    i = l;
    j = l << 1;
    while (j <= ir) {
      if (j < ir && ra[j] < ra[j+1]) { ++j; }
      if (rra < ra[j]) {
        ra[i] = ra[j];
        j += (i = j);
      } else {
        j = ir + 1;
      }
    }
    ra[i] = rra;
  }
}

