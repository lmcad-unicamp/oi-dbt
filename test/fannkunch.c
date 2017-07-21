/*
 * The Computer Lannguage Shootout
 * http://shootout.alioth.debian.org/
 * Contributed by Heiner Marxen
 *
 * "fannkuch"	for C gcc
 *
 * $Id: fannkuch-gcc.code,v 1.51 2008-03-06 02:23:27 igouy-guest Exp $
 */

#define Int	int
#define Aint	int

  static long
fannkuch( int n )
{
  long	flips;
  long	flipsMax;
  Int		r;
  Int		i;
  Int		k;
  Int		didpr;
  const Int	n1	= n - 1;

  if( n < 1 ) return 0;

  Aint perm[n];
  Aint perm1[n];
  Aint count[n];

  for( i=0 ; i<n ; ++i ) perm1[i] = i;	/* initial (trivial) permu */

  r = n; didpr = 0; flipsMax = 0;
  for(;;) {
    if( didpr < 30 ) {
      ++didpr;
    }
    for( ; r!=1 ; --r ) {
      count[r-1] = r;
    }

#define XCH(x,y)	{ Aint t_mp; t_mp=(x); (x)=(y); (y)=t_mp; }

    if( ! (perm1[0]==0 || perm1[n1]==n1) ) {
      flips = 0;
      for( i=1 ; i<n ; ++i ) {	/* perm = perm1 */
        perm[i] = perm1[i];
      }
      k = perm1[0];		/* cache perm[0] in k */
      do {			/* k!=0 ==> k>0 */
        Int	j;
        for( i=1, j=k-1 ; i<j ; ++i, --j ) {
          XCH(perm[i], perm[j])
        }
        ++flips;
        /*
         * Now exchange k (caching perm[0]) and perm[k]... with care!
         * XCH(k, perm[k]) does NOT work!
         */
        j=perm[k]; perm[k]=k ; k=j;
      }while( k );
      if( flipsMax < flips ) {
        flipsMax = flips;
      }
    }

    for(;;) {
      if( r == n ) {
        return flipsMax;
      }
      {
        Int	perm0 = perm1[0];
        i = 0;
        while( i < r ) {
          k = i+1;
          perm1[i] = perm1[k];
          i = k;
        }
        perm1[r] = perm0;
      }
      if( (count[r] -= 1) > 0 ) {
        break;
      }
      ++r;
    }
  }
}

  int
main( int argc, char* argv[] )
{
  int		n = 7;
  return fannkuch(n);
}
