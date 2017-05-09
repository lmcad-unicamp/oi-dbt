
int main(int argc, char *argv[]) {
  int n = 1000;
  int i, k, x[1000], y[1000];

  // emulando calloc
  for (i = 0; i < n; ++i) {
    x[i] = 0;
    y[i] = 0;
  }

  for (i = 0; i < n; i++) {
    x[i] = i + 1;
  }
  for (k=0; k<10000; k++) {
//    for (i = n-1; i >= 0; i--) {
    for (i = 0; i < n; i++) {
      y[i] += x[i];
    }
  }

  return y[0] + y[n-1];
}
