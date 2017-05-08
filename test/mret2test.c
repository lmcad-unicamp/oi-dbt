
int main() {
  int a = 10;
  int i = 0;
  i+=a;
  i *= a;
  for (; i < 300; i++) {
    if (i < 115)
      a += 10 * i;
    else
      a -= i;
  }
  return a;
}
