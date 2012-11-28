#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/select.h>
#include <unistd.h>


#include <curl/curl.h>
#include <jansson.h>

#include <sqlite3.h>
#include <SDL/SDL.h>
#include <libconfig.h>

#include "structures.h"
#include "net.h"
#include "vk_errors.h"
#include "tree.h"
#include "global-vars.h"

#define MAX_MESSAGE_LEN 1000



int persistent;

int limit;
int offset;
int reverse;



char *current_error;
int current_error_code;



config_t conf;

void vk_error (int error_code, const char *format, ...) {
  current_error_code = error_code;
  va_list l;
  va_start (l, format);
  static char buf[10000];
  vsnprintf (buf, 9999, format, l);
  if (current_error) { free (current_error); }
  current_error = strdup (buf);
  if (verbosity) {
    fprintf (stderr, "%s\n", current_error);
  }
}

void vk_log (int level, const char *format, ...) {
  if (level <= verbosity) {
    va_list l;
    va_start (l, format);
    vfprintf (stderr, format, l);
  }
}

int wait_all (void) {
  int t = 0;
  while ((t = work_read_write ()) == 0);
  return t;
}

void usage_auth (void) {
  printf ("vkconcli auth <username>\n");
  exit (ERROR_COMMAND_LINE);
}



int is_linebreak (char c);

int act_auth (char **argv UNUSED, int argc) {
  if (argc != 0) {
    usage_auth ();
  }
  while (!username) {
    printf ("Login (email): ");
    if (scanf ("%as", &username) != 1) {
      username = 0;
    }
    if (username && !strlen (username)) {
      free (username);
      username = 0;
    }
  }
  while (!password) {
    password = getpass ("Password: ");
    if (!strlen (password)) {
      free (password);
      password = 0;
    }
  }
  if (aio_auth (username, password) < 0) { return _ERROR; }
  return wait_all (); 
}

void usage_msg_read (void) {
  printf ("vkconcli msg read [in|out]\n");
  exit (ERROR_COMMAND_LINE);
}

int act_msg_read (char **argv, int argc) {
  if (argc != 1) {
    usage_msg_read ();
  }
  int out = 0;
  if (!strcmp (*argv, "in") || !strcmp (*argv, "inbox")) {
    out = 0;
  } else if (!strcmp (*argv, "out") || !strcmp (*argv, "outbox")) {
    out = 1;
  } else {
    usage_msg_read ();
  }

  if (aio_msgs_get (offset, limit, reverse, out) < 0) {
    return _ERROR;
  }
  while (work_read_write () == 0);
  return 0;
}

void usage_msg_send (void) {
  printf ("vkconcli msg send <recipient_id>\n");
  exit (ERROR_COMMAND_LINE);
}

void usage_wall_post (void) {
  printf ("vkconcli wall post <recipient_id>\n");
  exit (ERROR_COMMAND_LINE);
}

char *read_msg (void) {
  static char msg_buf[MAX_MESSAGE_LEN + 100];
  int cur_len = 0;
  msg_buf[0] = 0;
  while (!feof (stdin) && cur_len < MAX_MESSAGE_LEN) {
    msg_buf[cur_len ++] = fgetc (stdin);
    if (msg_buf[cur_len - 1] == -1) {
      cur_len --;
    }  
  }
  msg_buf[cur_len] = 0;
  return msg_buf;
}

int act_msg_send (char **argv, int argc) {
  if (argc != 1) {
    usage_msg_read ();
  }
  if (aio_msg_send (atoi (argv[0]), read_msg ()) < 0) { return _ERROR; }
  return wait_all ();
}

int act_wall_post (char **argv, int argc) {
  if (argc != 1) {
    usage_wall_post ();
  }
  if (aio_wall_post (atoi (argv[0]), read_msg ()) < 0) { return _ERROR; }
  return wait_all ();
}

void usage_msg (void) {
  printf ("vkconcli msg [read | send]\n");
  exit (ERROR_COMMAND_LINE);
}


void usage_user (void) {
  printf ("vkconcli user <id>\n");
  exit (ERROR_COMMAND_LINE);
}

void usage_wall (void) {
  printf ("vkconcli wall [post]\n");
  exit (ERROR_COMMAND_LINE);
}

int act_msg (char **argv, int argc) {
  if (argc <= 0) {
    usage_msg ();
  }
  if (!strcmp (argv[0], "read")) {
    return act_msg_read (argv + 1, argc - 1);
  }
  if (!strcmp (argv[0], "send")) {
    return act_msg_send (argv + 1, argc - 1);
  }
  usage_msg ();
  return 0; 
}

int act_user (char **argv, int argc) {
  if (argc <= 0 || argc > 100) {
    usage_user ();
  }
  static int q[1000];
  int i;
  for (i = 0; i < argc; i++) {
    q[i] = atoi (argv[i]);
    if (q[i] <= 0 || q[i] >= 1000000000) {
      usage_user ();
    }
  }

  if (aio_profiles_get (argc, q, 0) < 0) { return _ERROR; }
  while (work_read_write () == 0);
  return 0;
}

int act_wall (char **argv, int argc) {
  if (argc <= 0) {
    usage_wall ();
  }
  if (!strcmp (*argv, "post")) {
    return act_wall_post (argv + 1, argc - 1);
  }
  usage_wall ();
  return ERROR_COMMAND_LINE;
}

void usage_act (void) {
  printf ("vkconcli <action>. Possible actions are:\n");
  printf ("\tauth\n");
  printf ("\tmsg (message, messages, u)\n");
  printf ("\tuser (friend, u, users, friends, profiles)\n");
  printf ("\twall\n");
  exit (ERROR_COMMAND_LINE);
}

int act (const char *str, char **argv, int argc) {
  if (!strcmp (str, "auth")) {
    return act_auth (argv, argc);
  }
  if (!strcmp (str, "msg") || !strcmp (str, "message") || !strcmp (str, "messages") || !strcmp (str, "m")) {
    return act_msg (argv, argc);
  }
  if (!strcmp (str, "friend") || !strcmp (str, "user") || !strcmp (str, "u")) {
    return act_user (argv, argc);
  }
  if (!strcmp (str, "wall")) {
    return act_wall (argv, argc);
  }

  usage_act ();
  return 1;
}

void usage (void) {
  printf ("vkconcli [-v] [-h] <action>\n");
  printf ("\t-v: increase verbosity level by 1\n");
  printf ("\t-h: print this help and exit\n");
  printf ("\t-l: limit. Different things\n");
  printf ("\t-o: offset. Different things\n");
  printf ("\t-a: specify access token\n");
  printf ("\t-R: print in reverse order\n");
  printf ("\t-D: database souce [default $HOME/.vkdb]\n");
  printf ("\t-S: disable sql\n");
  printf ("\t-N: disable net\n");
  printf ("\t-M: set max depth\n");
  exit (ERROR_COMMAND_LINE);
}

#define STD_BUF_SIZE (1 << 20)
static char std_buf[STD_BUF_SIZE + 1];
int std_buf_pos;
int cur_pos;
char *cur_token;
int cur_token_len;
#define TOKEN_NEED_MORE_BYTES -1
#define TOKEN_ERROR -2
#define TOKEN_OK 1 
#define TOKEN_UNEXPECTED -3

#define WORK_CONSOLE 1
#define WORK_NET 2

#define cur_char std_buf[cur_pos]

int is_whitespace (char c) {
  return (c == 32 || c == 10 || c == 13 || c == 9 || c == 11 || c == 12);
}

int is_linebreak (char c) {
  return (c == 10 || c == 13);
}

int next_token (void) {
  while (is_whitespace (cur_char)) { cur_pos ++; }
  if (!cur_char) { return TOKEN_NEED_MORE_BYTES; }
  char c = cur_char;
  char *p = std_buf + cur_pos;
  if (c != '"') {
    while (!is_whitespace (cur_char) && cur_char) { cur_pos ++; }
    if (!cur_char) return TOKEN_NEED_MORE_BYTES;
    cur_token = p;
    cur_token_len = std_buf + cur_pos - p;
    return TOKEN_OK;
  } else {
    if (!std_buf[cur_pos + 1] || !std_buf[cur_pos + 2]) return TOKEN_NEED_MORE_BYTES;
    if (std_buf[cur_pos + 1] == '"' && std_buf[cur_pos + 2] == '"') {
      p += 3;
      cur_pos += 3;
      while (cur_char && memcmp ("\"\"\"", std_buf + cur_pos, 3)) { cur_pos ++; }
      if (!cur_char) { return TOKEN_NEED_MORE_BYTES; }
      cur_token = p;
      cur_token_len = std_buf + cur_pos - p;
      cur_pos += 3;
      return TOKEN_OK;
    } else {
      p ++;
      cur_pos ++;
      while (!is_linebreak (cur_char) && cur_char != '"' && cur_char) { cur_pos ++; }
      if (!cur_char) { return TOKEN_NEED_MORE_BYTES; }
      if (cur_char == '"') {
        cur_token = p;
        cur_token_len = std_buf + cur_pos - p;
        cur_pos ++;
        return TOKEN_OK;
      }
      return TOKEN_ERROR;
    }
  }
}

#define NT \
  { \
    int t = next_token ();\
    if (t != TOKEN_OK) { return t;}\
  }

int work_console_msg_to (int id) {
  NT;
  char *s = strndup (cur_token, cur_token_len);
  //vk_msg_send (id, s);
  if (aio_msg_send (id, s) < 0) {
    printf ("Not sent.\n");
    return _ERROR;
  }
  printf ("Msg to %d successfully sent:\n---\n%s\n---\n", id, s);
  free (s);
  return TOKEN_OK;
}

int work_console_msg (void) {
  NT;
  int n = atoi (cur_token);
  if (n <= 0 || n >= 1000000000) { return TOKEN_UNEXPECTED; }
  return work_console_msg_to (n);
}

int work_console_main (void) {
  NT;
  if (cur_token_len == 3 && !strncmp ("msg", cur_token, 3)) {
    return work_console_msg ();
  }
  return TOKEN_UNEXPECTED;
}

void work_console (void) {
  int x = read (STDIN_FILENO, std_buf + std_buf_pos, STD_BUF_SIZE - std_buf_pos);
  if (x <= 0) {
    return;
  }
  std_buf_pos += x;
  std_buf[std_buf_pos] = 0;
  int r = work_console_main ();
  if (r != TOKEN_NEED_MORE_BYTES) {
    memcpy (std_buf, std_buf + cur_pos, std_buf_pos - cur_pos + 1);
    std_buf_pos -= cur_pos;
    cur_pos = 0;
  } else {
    cur_pos = 0;
  }
}

extern CURLM *multi_handle;
int poll_work (void) {
  static fd_set fdr, fdw, fde;
  FD_ZERO (&fdr);
  FD_ZERO (&fdw);
  FD_ZERO (&fde);
  FD_SET (STDIN_FILENO, &fdr);
  int max_fd = 0;
  int e;
  if ((e = curl_multi_fdset (multi_handle, &fdr, &fdw, &fde, &max_fd)) != CURLE_OK) {
    vk_error (ERROR_CURL, "Curl_error %d: %s", e, curl_multi_strerror (e));
    return 0;
  }
  if (max_fd < 0) { max_fd = 0; }
  long timeout = 1000;
  if ((e = curl_multi_timeout (multi_handle, &timeout)) != CURLE_OK) {
    vk_error (ERROR_CURL, "Curl_error %d: %s", e, curl_multi_strerror (e));
    return 0;
  }
  if (timeout < 0) { timeout = 1000; }

  struct timeval t = {
    .tv_sec = timeout / 1000,
    .tv_usec = timeout % 1000
  };
  if (select (max_fd + 1, &fdr, &fdw, &fde, &t) < 0) {
    vk_error (ERROR_SYS, "System error %m");
  }
  int r = 0;
  if (FD_ISSET (0, &fdr)) { r |= WORK_CONSOLE; }
  FD_CLR (0, &fdr);
  r |= WORK_NET;
  return r;
}

int loop (void) {
  aio_longpoll ();
  while (1) {
    int x = poll_work ();
    if (x & WORK_CONSOLE) { work_console (); }
    if (x & WORK_NET) { work_read_write (); }
  }
  return 0;
}

int loop2 (void) {
  while (1) {
    int x = poll_work ();
    if (x & WORK_CONSOLE} { work_console (); }
    if (x & WORK_NET) { work_net (); }
    if (!(x & WORK_CONSOLE) && !working_handle_count) { return 0; }
  }
}

char *makepath (const char *path) {
  assert (path);
  if (*path == '/') { return strdup (path); }
  else { 
    char *s; 
    const char *h = getenv ("HOME");
    assert (asprintf (&s, "%s/%s", h, path) >= 0);
    return s;
  }
}

int load_access_token (void) {
  if (access_token) { return 1;}
  config_setting_t *conf_setting;
  if (!access_token) {
    conf_setting = config_lookup (&conf, "access_token");
    if (conf_setting) {
      access_token = strdup (config_setting_get_string (conf_setting));
      assert (access_token);
    }
    conf_setting = config_lookup (&conf, "access_token_file");
    if (conf_setting) {
      access_token_file = makepath (config_setting_get_string (conf_setting));
      if (!access_token) {
        FILE *f = fopen (access_token_file, "rt");
        if (f) {
          if (fscanf (f, "%as", &access_token) < 1) {
            access_token = 0;
          }
          fclose (f);
        }
      }
    }
  }
  if (!access_token) {
    access_token = getenv ("vk_access_token");
    if (!access_token) {
      vk_error (ERROR_NO_ACCESS_TOKEN, "No access token");
      return 0;
    }
    access_token = strdup (access_token);    
  }
  return 1;
}

int main (int argc, char **argv) {
  char c;
  int connections = 10;
  while ((c = getopt (argc, argv, "vhl:o:a:RD:M:Pc:A:S:N:m:u:")) != -1) {
    switch (c) {
      case 'u':
        username = strdup (optarg);
        break;
      case 'p':
        password = strdup (optarg);
        break;
      case 'v': 
        verbosity ++;
        break;
      case 'l':
        limit = atoi (optarg);
        break;
      case 'o':
        offset = atoi (optarg);
        break;
      case 'a':
        access_token = optarg;
        break;
      case 'R':
        reverse ++;
        break;
      case 'D':
        db_file_name = optarg;
        break;
      case 'A':
        if (!strcmp (optarg, "disable")) {
          disable_audio = 1;
        } else if (!strcmp (optarg, "enable")) {
          disable_audio = 0;
        } else {
          fprintf (stderr, "enable/disable expected\n");
          return 2;
        }
        break;
      case 'S':
        if (!strcmp (optarg, "disable")) {
          disable_sql = 1;
        } else if (!strcmp (optarg, "enable")) {
          disable_sql = 0;
        } else {
          fprintf (stderr, "enable/disable expected\n");
          return 2;
        }
        break;
      case 'N':
        if (!strcmp (optarg, "disable")) {
          disable_net = 1;
        } else if (!strcmp (optarg, "enable")) {
          disable_net = 0;
        } else {
          fprintf (stderr, "enable/disable expected\n");
          return ERROR_COMMAND_LINE;
        }
        break;
      case 'M':
        max_depth = atoi (optarg);
        break;
      case 'P':
        persistent ++;
        break;
      case 'm':
        connections = atoi (optarg);
        break;
      case 'c':
        config_file_name = optarg;
        break;
      case 'h':
      default:
        usage ();
    }
  }

  config_file_name = makepath (config_file_name);
  config_init (&conf);

  if (config_read_file (&conf, config_file_name) != CONFIG_TRUE) {
    fprintf (stderr, "error parsing config: %s\n", config_error_text (&conf));
    return ERROR_CONFIG;
  }

  config_setting_t *conf_setting;
  if (disable_net == -1) {
    conf_setting = config_lookup (&conf, "disable_net");
    if (!conf_setting) {
      disable_net = 0;
    } else {
      disable_net = config_setting_get_bool (conf_setting);
    }
  }

  if (disable_sql == -1) {
    conf_setting = config_lookup (&conf, "disable_sql");
    if (!conf_setting) {
      disable_sql = 0;
    } else {
      disable_sql = config_setting_get_bool (conf_setting);
    }
  }

  if (disable_audio == -1) {
    conf_setting = config_lookup (&conf, "disable_audio");
    if (!conf_setting) {
      disable_audio = 0;
    } else {
      disable_audio = config_setting_get_bool (conf_setting);
    }
  }

  conf_setting = config_lookup (&conf, "default_history_limit");
  if (conf_setting) {
    default_history_limit = config_setting_get_int (conf_setting);
  }

  if (!username) {
    conf_setting = config_lookup (&conf, "username");
    if (conf_setting) {
      username = strdup (config_setting_get_string (conf_setting));
    }
  }

  if (!password) {
    conf_setting = config_lookup (&conf, "password");
    if (conf_setting) {
      username = strdup (config_setting_get_string (conf_setting));
    }
  }

  if (!disable_net) {
    load_access_token ();
  }
  
  if (!disable_sql && !db_file_name) {
    conf_setting = config_lookup (&conf, "db_file");
    if (conf_setting) {
      db_file_name = strdup (config_setting_get_string (conf_setting));
    }
  }
  if (db_file_name) {
    db_file_name = makepath (db_file_name);
  }
  argc -= optind;
  argv += optind;
  
  assert (argc >= 0);

  if (!disable_net) {
    if (connections <= 1) { connections = 2; }
    if (connections >= 1000) { connections = 1000; }
    if (vk_net_init (1, connections) < 0) {
      fprintf (stderr, "Error #%d: %s\n", current_error_code, current_error);
      return current_error_code;
    }
  }
  if (!disable_sql) {
    if (vk_db_init (db_file_name) < 0) {
      fprintf (stderr, "Error #%d: %s\n", current_error_code, current_error);
      return current_error_code;
    }
  }
  if (!disable_audio) {
    if (SDL_Init (SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
      fprintf (stderr, "Sdl init error %s\n", SDL_GetError ());
      return ERROR_SDL;
    }
  }
  if (persistent) {
    if (disable_net || disable_sql) {
      fprintf (stderr, "In persistent mode sql and net should be on\n");
      return ERROR_COMMAND_LINE;
    }
    if (loop () < 0) {
      fprintf (stderr, "Error #%d: %s\n", current_error_code, current_error);
    }
  }
  if (!argc) {
    usage ();
  }
  if (act (*argv, argv + 1, argc - 1) < 0) {
    fprintf (stderr, "Error #%d: %s\n", current_error_code, current_error);
    return current_error_code;
  }
  return 0;
}
