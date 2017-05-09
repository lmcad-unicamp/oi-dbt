
int main() {
  int b = 300;
  int a = 10;
  int i = 0;
  i+=a;
  i *= a;
  for (; i < b; i++) {
    if (i < 120)
      a += 3;
    else
      a += 4;
  }
  return a;
}
