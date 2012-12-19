#ifdef HAVE_CONFIG_H
#  include "config.h"
#else
#  define ENABLE_LIBCONFIG
#endif
#define _GNU_SOURCE

#ifdef ENABLE_LIBCONFIG
#  include <libconfig.h>
#endif

#include "util_config.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "vk_errors.h"
#ifdef ENABLE_LIBCONFIG
config_t conf;
#endif

int verbosity;

char *access_token;
char *access_token_file;
char *username;
char *password;

int disable_sql = -1;
int disable_net = -1;
int disable_audio = -1;

char *config_file_name = DEFAULT_CONFIG_FILE_NAME;

char *db_file_name;

int default_history_limit = -1;
int max_depth = -1;

int working_handle_count;
int handle_count;

int connections = -1;

#ifdef ENABLE_LIBCONFIG

#define ANY_CONF_VAR(var,conf_path,default_val,not_set,suffix) \
  if (var == not_set) { \
    conf_setting = config_lookup (&conf, conf_path); \
    if (!conf_setting) {  \
      var = default_val; \
    } else { \
      var = config_setting_get_ ## suffix (conf_setting); \
    } \
  } 


#define STR_CONF_VAR(var,conf_path,default_val) \
  if (var == 0) { \
    conf_setting = config_lookup (&conf, conf_path); \
    if (!conf_setting) {  \
      var = default_val ? strdup (default_val) : 0; \
    } else { \
      var = strdup (config_setting_get_string (conf_setting)); \
    } \
  } 

#define STR_CONF_VAR0(var,conf_path) \
  if (var == 0) { \
    conf_setting = config_lookup (&conf, conf_path); \
    if (!conf_setting) {  \
      var = 0; \
    } else { \
      var = strdup (config_setting_get_string (conf_setting)); \
    } \
  } 

#else
#define ANY_CONF_VAR(var,conf_path,default_val,not_set,suffix) \
  if (var == not_set) { \
    var = default_val; \
  } 


#define STR_CONF_VAR(var,conf_path,default_val) \
  if (var == 0) { \
    var = default_val ? strdup (default_val) : 0; \
  } 

#define STR_CONF_VAR0(var,conf_path) \
  if (var == 0) { \
    var = 0; \
  } 

#endif

char *makepath (const char *path) {
  assert (path);
  if (*path == '/') { 
    return strdup (path); 
  } else { 
    char *s; 
    const char *h = getenv ("HOME");
    assert (asprintf (&s, "%s/%s", h, path) >= 0);
    return s;
  }
}

#define BOOL_CONF_VAR(a,b,c) ANY_CONF_VAR (a, b, c, -1, bool)
#define INT_CONF_VAR(a,b,c) ANY_CONF_VAR (a, b, c, -1, int)
void read_config (void) {
#ifdef ENABLE_LIBCONFIG
  config_file_name = makepath (config_file_name);
  config_setting_t *conf_setting;
  config_init (&conf);

  if (config_read_file (&conf, config_file_name) != CONFIG_TRUE) {
    vk_critical_error (ERROR_CONFIG, "error parsing config `%s`: %s\n", config_file_name, config_error_text (&conf));
  }
#endif
  BOOL_CONF_VAR (disable_net, "disable_net", 0);
  BOOL_CONF_VAR (disable_sql, "disable_sql", 0);
  BOOL_CONF_VAR (disable_audio, "disable_audio", 0);
  INT_CONF_VAR (default_history_limit, "default_history_limit", 100);
  STR_CONF_VAR0 (username, "username");
  STR_CONF_VAR0 (password, "password");
  STR_CONF_VAR0 (access_token, "access_token");
  STR_CONF_VAR0 (access_token_file, "access_token_file");
  if (access_token_file) {
    access_token_file = makepath (access_token_file); 
  }
  STR_CONF_VAR (db_file_name, "db_file_name", DEFAULT_DB_FILE_NAME);
  if (db_file_name) {
    db_file_name = makepath (db_file_name);
  }
  INT_CONF_VAR (connections, "connections", 10);
  if (connections <= 1) { connections = 2; }
  if (connections >= 1000) { connections = 1000; }
}
