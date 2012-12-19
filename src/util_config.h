#ifndef __UTIL_CONFIG_H__
#define __UTIL_CONFIG_H__

#define DEFAULT_CONFIG_FILE_NAME ".vkconcli/config"
#define DEFAULT_DB_FILE_NAME ".vkconcli/db"

extern int verbosity;

extern char *access_token;
extern char *access_token_file;

extern char *username;
extern char *password;

extern int disable_sql;
extern int disable_net;
extern int disable_audio;

extern char *config_file_name;
extern char *db_file_name;

extern int default_history_limit;
extern int max_depth;

extern int working_handle_count;
extern int handle_count;

extern int connections;

void read_config (void);
#endif
