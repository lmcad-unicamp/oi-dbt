#include <stdio.h>

int main() {
#define SIZE 100
  unsigned long long factorial;
  for (int i = 0; i < 100; i++) {
    int n, i;
    factorial = 1;

    n = 12;

    for(i=1; i<=n; ++i) {
      factorial *= i;              // factorial = factorial*i;
    }
  }

  return (factorial-100);
}
