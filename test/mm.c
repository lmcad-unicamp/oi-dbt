int main() {
  int m, n, p, q, c, d, k, sum = 0;
  #define size 200
  int first[size][size], second[size][size], multiply[size][size];

  m = size;
  p = size;
  q = size;
  n = size;

  for (c = 0; c < m; c++)
    for (d = 0; d < n; d++)
      first[c][d] = (1+c) * (d+1);


  unsigned acc = 0;
  if (n != p)
    return -1;
  else
  {

    for (c = 0; c < p; c++)
      for (d = 0; d < q; d++)
        second[c][d] = (4+c) * (d+2);

    for (c = 0; c < m; c++) {
      for (d = 0; d < q; d++) {
        for (k = 0; k < p; k++) 
          sum += first[c][k]*second[k][d];

        multiply[c][d] = sum;
        sum = 0;
      }
    }

    for (c = 0; c < m; c++) 
      for (d = 0; d < q; d++) {
        acc += multiply[c][d];
      }


  }
  return acc;
}
