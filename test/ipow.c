unsigned long long ipow(int base, int exp) {
#define SIZE 100
  unsigned long long result = 1;
  for (int i = 0; i < SIZE; i++) {
    result = 1;
    while (exp) {
      if (exp & 1)
        result *= base;
      exp >>= 1;
      base *= base;
    }
  }

  return result;
}

int main() {
  return ipow(2, 54) - ipow(-3, 23);
}
