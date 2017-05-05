/* -*- mode: c -*-
 * $Id: nestedloop.c 36673 2007-05-03 16:55:46Z laurov $
 * http://www.bagley.org/~doug/shootout/
 */

int main() {
  #define LENGTH 100
  int n = 1;
  int x = LENGTH;
  int a;

  for (a=0; a<n; a++)
    x /= 2;

  for (a=0; a < n; a++)
    x++;

  for (a=0; a<n; a++)
    x--;

  for (a=0; a<n; a++)
    x = x << 1;

  for (a=0; a<n; a++)
    x = x >> 2;

  for (a=0; a<n; a++) {
    x += x;
  }

  for (a=0; a<n; a++)
    x *= 2;

  for (a=0; a<n; a++)
    x -= a;
  
  int y = 1;
  for (a=0; a<n; a++)
    x /= y;
//  y += x;

  return x;
}


