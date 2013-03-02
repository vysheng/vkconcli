#ifndef __UTIL_DB_H__
#define __UTIL_DB_H__
extern sqlite3 *db_handle;

int ct_db_callback (void *, int, char **, char **);

#endif
