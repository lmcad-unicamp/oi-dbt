/* testc - Test adpcm coder */

#include "adpcm.h"
#include <stdio.h>
#include <fcntl.h>

struct adpcm_state state;

#define NSAMPLES 1000

char	abuf[NSAMPLES/2];
short	sbuf[NSAMPLES];

main(int argc, char** argv) {
  int n;

  int file = open(argv[1], O_RDONLY);
  while(1) {
	n = read(file, sbuf, NSAMPLES*2);
	if ( n < 0 ) {
	    perror("input file");
	    exit(1);
	}
	if ( n == 0 ) break;
	adpcm_coder(sbuf, abuf, n/2, &state);
	write(1, abuf, n/4);
    }
    fprintf(stderr, "Final valprev=%d, index=%d\n",
	    state.valprev, state.index);
    exit(0);
}
