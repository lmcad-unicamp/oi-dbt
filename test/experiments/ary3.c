#define LENGTH 100000

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
