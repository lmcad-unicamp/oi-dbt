/* -*- mode: c -*-
 * $Id: nestedloop.c 36673 2007-05-03 16:55:46Z laurov $
 * http://www.bagley.org/~doug/shootout/
 */

int main(int argc, char *argv[]) {
#define LENGTH 21
  int n = LENGTH;
  int a, b, c, d, e, f, x=0;

  for (a=0; a<n; a++)
    for (b=0; b<n; b++)
      for (c=0; c<n; c++)
        for (d=0; d<n; d++)
          for (e=0; e<n; e++)
            for (f=0; f<n; f++)
              x++;

    return(x);
}


