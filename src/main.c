#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


#include <curl/curl.h>
int verbosity;

void usage_auth (void) {
  printf ("vkconcli auth <username> <password>\n");
  exit (1);
}

int act_auth (char **argv, int argc) {
  if (argc != 2) {
    usage_auth ();
  }
  //snprintf ("
  return 0; 
}


void usage_act (void) {
  printf ("vkconcli <action>. Possible actions are:\n");
  printf ("\tauth\n");
  exit (1);
}

int act (const char *str, char **argv, int argc) {
  if (!strcmp (str, "auth")) {
    return act_auth (argv, argc);
  }
  usage_act ();
  return 1;
}

void usage (void) {
  printf ("vkconcli [-v] [-h] <action>\n");
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

  argc -= optind;
  argv += optind;
  
  assert (argc >= 0);
  if (!argc) {
    usage ();
  }


  if (curl_global_init (CURL_GLOBAL_ALL)) {
    exit (2);
  }
  return act (*argv, argv + 1, argc - 1);
}
