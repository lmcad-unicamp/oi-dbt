int test(int n) {
  volatile int x = n;
  volatile int a;

  for (a=0; a < n; a++) {
    x /= n;
    x++;
    x--;
    x = x << 2;
    x = x >> 2;
    x += x;
    x *= a;
    x -= a;
  }

  return x;
}

int main() {
  volatile int n = 200;
  volatile int j = 0;
  for (int i = 0; i < n; i++) 
    for (int k = 0; k < n; k++) 
      for (int z = 0; z < n; z++) 
        j += z-i;

  for (int i = 0; i < n; i++) { 
    j--;
    for (int k = 0; k < n; k++) {  
      for (int z = 0; z < n; z++) {
        j += z-i;
      }
      j++;
    }
    test(i);
  }

  for (int z = 0; z < n; z++) {
    j += z + j;
  }

  for (int z = 0; z < n; z++) {
    j -= z - 5 + 2;
  }

  test(100);
  return j;
}
