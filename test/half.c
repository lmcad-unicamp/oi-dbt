#define LENGTH 150

short x[LENGTH];

int main(int argc, char *argv[]) {
  int n = LENGTH;
  int i, k;
  unsigned short y[LENGTH];

  for (i = 0; i < n; i++) 
    y[i] = i;

  for (i = 0; i < n; i++) 
    x[i] = (short) i + 1;

  for (k=0; k<1000; k++) 
    for (i = n-1; i >= 0; i--) { 
      x[i] += y[i];
      y[i] += x[i];
    }

  return y[n-50] + x[n-25];
}
