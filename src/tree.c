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

int vk_alias_add (const char *id, const char *result) {
  char *q = sqlite3_mprintf ("INSERT INTO aliases (id, result) VALUES (%Q, %Q);", id, result);
  if (verbosity >= 4) {
    fprintf (stderr, "%s\n", q);   
  }
  char *e = 0;

  if (sqlite3_exec (db_handle, q, 0, 0, &e) != SQLITE_OK || e) {
    vk_error (ERROR_SQLITE, "SQLite error %s\n", e);
    return _ERROR;
  }

  sqlite3_free (q);
  
  return 0;
}

int set_string_callback (void *data, int size, char **results, char **fields UNUSED) {
  assert (!*(char **)data);
  assert (size == 1);
  *(char **)data = strdup (*results);
  return 0;
}

char *vk_alias_get (const char *id) {
  char *q = sqlite3_mprintf ("SELECT result FROM aliases WHERE id=%Q;", id);
  if (verbosity >= 4) {
    fprintf (stderr, "%s\n", q);   
  }
  char *e = 0;

  char *result = 0;
  if (sqlite3_exec (db_handle, q, set_string_callback, &result, &e) != SQLITE_OK || e) {
    vk_error (ERROR_SQLITE, "SQLite error %s\n", e);
    return 0;
  }

  sqlite3_free (q);
  
  return result;
}

int vk_check_table_aliases (void) {
  char *q = sqlite3_mprintf ("%s", "CREATE TABLE IF NOT EXISTS aliases (id VARCHAR NOT NULL UNIQUE, result VARCHAR);");
  assert (q);
  if (verbosity >= 4) {
    fprintf (stderr, "%s\n", q);   
  }

  char *e = 0;
  if (sqlite3_exec (db_handle, q, 0, 0, &e) != SQLITE_OK || e) {
    vk_error (ERROR_SQLITE, "SQLite error %s\n", e);
    return _ERROR;
  }

  sqlite3_free (q);
  
  return 0;
}

int db_check_tables (void) {
  if (vk_check_table_messages () < 0) { return _ERROR; }
  if (vk_check_table_users () < 0) { return _ERROR; }
  if (vk_check_table_aliases () < 0) { return _ERROR; }
  return 0;
}

int db_open (const char *filename) {
  vk_log (1, "Opening database %s\n", filename);
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
    if (snprintf (db_query_buf, DB_QUERY_BUF_SIZE, "%s/.vkconcli/db", h) >= DB_QUERY_BUF_SIZE) {
      vk_error (ERROR_BUFFER_OVERFLOW, "Path to db is tooooooo long");
      return _FATAL_ERROR;
    }
    return db_open (db_query_buf);
  } else {
    return db_open (filename);
  }
}
