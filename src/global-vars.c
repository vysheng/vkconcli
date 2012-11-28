int verbosity;

char *access_token;
char *access_token_file;
char *username;
char *password;

int disable_sql = -1;
int disable_net = -1;
int disable_audio = -1;

char *config_file_name = ".vkconcli/config";
char *db_file_name = ".vkconcli/db";

int default_history_limit = 100;
int max_depth = 2;

int working_handle_count;
int handle_count;
