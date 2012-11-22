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
  if (vk_check_table_messages () < 0) { return _ERROR; }
  if (vk_check_table_users () < 0) { return _ERROR; }
  return 0;
}

int db_open (const char *filename) {
  if (verbosity >= 1) {
    fprintf (stderr, "Opening database %s\n", filename);
  }
  int r = sqlite3_open (filename, &db_handle);
  if (r != SQLITE_OK) {
    vk_error (ERROR_SQLITE, "SQLite error %s", db_handle ? sqlite3_errmsg (db_handle) : "Critical fail");
    return _FATAL_ERROR;
  }
  return db_check_tables ();
}

int vk_db_init (const char *filename) {
  if (!filename) {
    const char *h = getenv ("HOME");
    if (!h) {
      vk_error (ERROR_NO_DB, "Can not create name for db: no home directory");
      return _FATAL_ERROR;
    }
    if (snprintf (db_query_buf, DB_QUERY_BUF_SIZE, "%s/.vkdb", h) >= DB_QUERY_BUF_SIZE) {
      vk_error (ERROR_BUFFER_OVERFLOW, "Path to db is tooooooo long");
      return _FATAL_ERROR;
    }
    return db_open (db_query_buf);
  } else {
    return db_open (filename);
  }
}
