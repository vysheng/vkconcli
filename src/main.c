#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int verbosity;

void usage (void) {
  printf ("vkconcli [-v] [-h] action\n");
  printf ("\t-v: increase verbosity level by 1\n");
  printf ("\t-h: print this help and exit\n");
  exit (1);
}

int main (int argc, char **argv) {
  char c;
  while ((c = getopt (argc, argv, "vh")) != -1) {
    switch (c) {
      case 'v': 
        verbosity ++;
        break;
      case 'h':
      default:
        usage ();
    }
  }

  return 0;
}
