#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <sqlite3.h>

#include "tree.h"
#include "vk_errors.h"

sqlite3 *db_handle;
char db_query_buf[DB_QUERY_BUF_SIZE];
extern int verbosity;

int db_check_tables (void) {
  assert (vk_check_table_messages () >= 0);
  assert (vk_check_table_users () >= 0);
  return 0;
}

int db_open (const char *filename) {
  if (verbosity >= 1) {
    fprintf (stderr, "Opening database %s\n", filename);
  }
  int r = sqlite3_open (filename, &db_handle);
  if (r != SQLITE_OK) {
    fprintf (stderr, "SQLite error %s\n", db_handle ? sqlite3_errmsg (db_handle) : "Critical fail");
    exit (ERROR_SQLITE);
  }
  assert (db_check_tables () >= 0);
  return 0;
}

int vk_db_init (const char *filename) {
  if (!filename) {
    const char *h = getenv ("HOME");
    if (!h) {
      exit (ERROR_NO_DB);
    }
    if (snprintf (db_query_buf, DB_QUERY_BUF_SIZE, "%s/.vkdb", h) >= DB_QUERY_BUF_SIZE) {
      exit (ERROR_BUFFER_OVERFLOW);
    }
    return db_open (db_query_buf);
  } else {
    return db_open (filename);
  }
}
