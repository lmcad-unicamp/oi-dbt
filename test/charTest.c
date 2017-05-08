
int main() {
  char s[1000];
  
  int a = 3;
  int b = 10;

  for (int i = 0; i < 1000; i++) {
    s[i] = 0;
    a += b;
    if (a > 10) b = 0;
  }

  for (int i = 0; i < 1000; i++) {
    s[i] += 1;
  }

  return s[a] + s[b] + s[b+a];
}
