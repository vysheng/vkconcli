int verbosity;

char *access_token;
char *access_token_file;
char *username;
char *password;

int disable_sql = -1;
int disable_net = -1;
int disable_audio = -1;

char *config_file_name = ".vkconcli/config";

//char *db_file_name = ".vkconcli/db";
char *db_file_name;

int default_history_limit = -1;
int max_depth = -1;

int working_handle_count;
int handle_count;

int connections = -1;
