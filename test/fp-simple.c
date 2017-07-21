int main() {
  double f = 1.1;
  float  d = 2.1;
  double res = 0;
  for(int i = 0; i < 100000; i++) {
    res += 1.0001 + f * d;
  }
  printf("%d\n", (int)res);
  return (int)res;
}
