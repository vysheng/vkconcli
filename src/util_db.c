#include <sqlite3.h>
#include "vkconcli.h"
#include <stdio.h>

int ct_db_callback (void *_ __attribute__ ((unused)), int argc, char **argv, char **cols) {
  if (verbosity >= 2) {
    int i;
    for (i = 0; i < argc; i++) {
      fprintf (stderr, "%s = %s\n", cols[i], argv[i] ? argv[i] : "NULL");
    }
    fprintf (stderr, "\n");
  }
  return 0;
}
