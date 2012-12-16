#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


#include <curl/curl.h>
#include <jansson.h>

#include "vk_errors.h"
#include "net.h"
#include "structures-auto.h"
#include "structures.h"
#include "global-vars.h"
#include "tree.h"

#define CLIENT_ID 2870218
#define CLIENT_ID_STR #CLIENT_ID
#define CLIENT_SECRET "R9hl0gmUCVAEqYHOYYtu"


#define TRY_CURL(f) { int _r = f; if (_r != CURLE_OK) { vk_error (ERROR_CURL, "Curl error: %s", curl_easy_strerror (_r)); return _ERROR;}}

CURL *curl_handle;
CURLM *multi_handle;
struct vk_curl_handle *all_handles;

//#define BUF_SIZE (1 << 23)
//char buf[BUF_SIZE];

struct curl_buf {
  int pos;
  int len;
  char *buf;
  int error;
};

struct vk_methods {
  int (*on_curl_fail)(struct vk_curl_handle *handle, int error_code);
  json_t *(*parse)(struct vk_curl_handle *handle);
  int (*on_parse_fail)(struct vk_curl_handle *handle);
  void *(*on_end)(struct vk_curl_handle *handle, json_t *ans);
  int (*finalize)(struct vk_curl_handle *handle, void *data);
  void (*free)(struct vk_curl_handle *handle, void *data);
};

struct vk_methods zero_methods;

struct vk_curl_handle {
  CURL *handle;
  char *query;
  struct curl_buf buf;
  long long flags;
  struct vk_methods methods;
  void *extra;
};

/* {{{ CURL handle */
struct vk_curl_handle *get_handle (void) {
  int i;
  for (i = 0; i < handle_count; i++) {
    if (!(all_handles[i].flags & 1)) {
      return &all_handles[i];
    }
  }
  return 0;
}

struct vk_curl_handle *vk_choose_by_handle (CURL *handle) {
  int i;
  for (i = 0; i < handle_count; i++) {
    if (handle == all_handles[i].handle) {
      return &all_handles[i];
    }
  }
  return 0;
}


int set_curl_url (CURL *curl_handle, const char *query) {
  TRY_CURL (curl_easy_setopt (curl_handle, CURLOPT_URL, query));
  return 0;
}

int query_perform (CURL *curl_handle, struct curl_buf *buf) {
  buf->error = 0;
  buf->pos = 0;
  TRY_CURL (curl_easy_setopt (curl_handle, CURLOPT_WRITEDATA, buf));
  TRY_CURL (curl_easy_perform (curl_handle));
  return 0;
}

int do_query (struct vk_curl_handle *handle, const char *query, struct vk_methods *methods, int flags, void *extra) {
  assert (handle);
  assert (!(handle->flags & 1));
  handle->query = strdup (query);
  handle->methods = methods ? *methods : zero_methods;
  TRY_CURL (curl_easy_setopt (handle->handle, CURLOPT_URL, query));
  handle->buf.error = 0;
  handle->buf.pos = 0;
  handle->flags = (handle->flags & 0xffffffff) | ((flags * 1ll) << 32);
  handle->extra = extra;
  TRY_CURL (curl_easy_setopt (handle->handle, CURLOPT_WRITEDATA, &handle->buf));
  TRY_CURL (curl_multi_add_handle (multi_handle, handle->handle));
  working_handle_count ++;
  handle->flags |= 1;
  return 0;
}
/* }}} */

/* {{{ Default curl methods */
int default_on_curl_fail (struct vk_curl_handle *handle UNUSED, int result) {
  vk_error (ERROR_CURL, "Curl error %d: %s\n", result, curl_easy_strerror (result));
  return _ERROR;
}

json_t *default_parse (struct vk_curl_handle *handle) {
  vk_log (3, "received answer %.*s\n", handle->buf.pos, handle->buf.buf);
  json_error_t error;
  json_t *ans = json_loadb (handle->buf.buf, handle->buf.pos, 0, &error);
  if (!ans) { 
    if (!error.text) {  
      vk_error (ERROR_PARSE_ANSWER, "Received invalid JSON"); 
    } else { 
      vk_error (ERROR_PARSE_ANSWER, "JSON parse error: %s", error.text); 
    } 
    return 0; 
  }

  vk_log (2, "Answer parsed\n"); 
  
  if (json_object_get (ans, "error")) {
    json_t *e = json_object_get (ans, "error"); 
    assert (e); 
    const char *s = json_object_get (e, "error_msg") ? json_string_value (json_object_get (e, "error_msg")) : ""; 
    vk_error (ERROR_PARSE_ANSWER, "Received error from vk: %s", s); 
    json_decref (ans); 
    return 0; 
  }

  return ans;
}

void default_free (struct vk_curl_handle *handle UNUSED, void *data) {
  if (data) { free (data); }
}

void do_nothing_free (struct vk_curl_handle *handle UNUSED, void *data UNUSED) {
}
/* }}} */

/* {{{ Longpoll methods */

char *longpoll_key;
char *longpoll_server;
int longpoll_ts;

void *vk_longpoll_request_aio (struct vk_curl_handle *handle UNUSED, json_t *ans) {
  if (!(ans = json_object_get (ans, "response"))) {
    vk_error (ERROR_UNEXPECTED_ANSWER, "No response field in answer");
    return 0;
  }
  if (!json_object_get (ans, "key") || !json_object_get (ans, "server") || !json_object_get (ans, "ts")) {
    vk_error (ERROR_UNEXPECTED_ANSWER, "No key/server/answer field in response for longpoll");
    return 0;
  }
  longpoll_key = strdup (json_string_value (json_object_get (ans, "key")));
  longpoll_server = strdup (json_string_value (json_object_get (ans, "server")));
  longpoll_ts = json_integer_value (json_object_get (ans, "ts"));
  aio_longpoll ();
  return (void *)-1l;
}

int vk_longpoll_request_finalize (struct vk_curl_handle *handle UNUSED, void *data) {
  assert (data == (void *)-1l);
  vk_log (1, "got longpoll server\n");
  return 0;
}

int aio_longpoll_request (void) {
  if (!access_token) {
    vk_error (ERROR_NO_ACCESS_TOKEN, "No access token");
    return _ERROR;
  }
  static char query[1001];  
  snprintf (query, 1000, "https://api.vk.com/method/messages.getLongPollServer?access_token=%s", access_token);

  struct vk_methods methods = {
    .parse = default_parse,
    .on_end = vk_longpoll_request_aio,
    .finalize = vk_longpoll_request_finalize,
    .free = do_nothing_free
  };
  struct vk_curl_handle *handle = get_handle ();
  if (!handle) { return _ERROR; }
  return do_query (handle, query, &methods, 0, 0);
}

void *vk_longpoll_aio (struct vk_curl_handle *handle UNUSED, json_t *ans) {
  if (json_object_get (ans, "failed")) {
    vk_log (1, "longpoll key expired\n");
    free (longpoll_key); longpoll_key = 0;
    free (longpoll_server); longpoll_server = 0;
    longpoll_ts = 0;
    aio_longpoll_request ();
    return (void *)-2l;
  }
  if (!json_object_get (ans, "ts") || !json_object_get (ans, "updates")) {
    vk_error (ERROR_UNEXPECTED_ANSWER, "No ts/updates in response from longpoll server");
    return 0;
  }
  longpoll_ts = json_integer_value (json_object_get (ans, "ts"));
  ans = json_object_get (ans, "updates");
  int n = json_array_size (ans);
  int i;
  for (i = 0; i < n; i++) {
    json_t *r = json_array_get (ans, i);
    if (!r) { continue; }
    int k = json_integer_value (json_array_get (r, 0));
    if (k == 4) {
      struct message *msg = vk_parse_message_longpoll (r);
      if (!msg) { return 0; }
      if (!disable_sql) {
        if (vk_db_insert_message (msg) < 0) {
          free (msg);
          return 0;
        }
      }
      print_message (0, msg);
    }
  }
  aio_longpoll ();
  return (void *)-1l;
}

int vk_longpoll_finalize (struct vk_curl_handle *handle UNUSED, void *data) {
  assert (data == (void *)-1l || data == (void *)-2l);
  vk_log (1, "got longpoll answer\n");
  return 0;
}

int aio_longpoll (void) {
  if (!access_token) {
    vk_error (ERROR_NO_ACCESS_TOKEN, "No access token");
    return _ERROR;
  }
  if (!longpoll_server || !longpoll_key) {
    return aio_longpoll_request ();
  }
  static char query[1001];
  snprintf (query, 1000, "http://%s?act=a_check&key=%s&ts=%d&wait=25&mode=2", longpoll_server, longpoll_key, longpoll_ts);

  struct vk_methods methods = {
    .parse = default_parse,
    .on_end = vk_longpoll_aio,
    .finalize = vk_longpoll_finalize,
    .free = do_nothing_free
  };
  struct vk_curl_handle *handle = get_handle ();
  if (!handle) { return _ERROR; }
  return do_query (handle, query, &methods, 0, 0);
}

/* }}} */

/* {{{ Auth methods */
void *vk_auth_aio (struct vk_curl_handle *handle UNUSED, json_t *ans) {
  if (!(ans = json_object_get (ans, "access_token"))) {
    vk_error (ERROR_UNEXPECTED_ANSWER, "No response field in answer");
    return 0;
  }
  char *s = strdup (json_string_value (ans));
  return s;
}

int vk_auth_finalize (struct vk_curl_handle *handle UNUSED, void *data) {
  if (access_token_file) {
    FILE *f = fopen (access_token_file, "wt");
    if (f) {
      fprintf (f, "%s", (char *)data);
    }
  } else {
    printf ("%s", (char *)data);
  }
  access_token = strdup (data);
  return 0;
}

int aio_auth (const char *user, const char *password) {
  assert (strlen (user) <= 100);
  assert (strlen (password) <= 100);
  static char query[1001];
  snprintf (query, 1000, "https://oauth.vk.com/token?grant_type=password&client_id=%d&client_secret=%s&username=%s&password=%s&scope=%d", CLIENT_ID, CLIENT_SECRET, user, password, 2 | 4096 | 8192 | 512);

  struct vk_methods methods = {
    .parse = default_parse,
    .on_end = vk_auth_aio,
    .finalize = vk_auth_finalize
  };
  struct vk_curl_handle *handle = get_handle ();
  if (!handle) { return _ERROR; }
  return do_query (handle, query, &methods, 0, 0);
}

/* }}} */

/* {{{ Get profiles methods */
void *vk_users_get_aio (struct vk_curl_handle *handle UNUSED, json_t *ans) {
  if (!(ans = json_object_get (ans, "response"))) {
    vk_error (ERROR_UNEXPECTED_ANSWER, "No response field in answer");
    return 0;
  }
  int l = json_array_size (ans);
  struct user **users = malloc (sizeof (void *) * (l + 1));
  users[0] = (void *)(long )(l);
  int i;
  for (i = 1; i <= l; i++) {
    users[i] = vk_parse_user (json_array_get (ans, i - 1));
    if (!users[i]) {
      int j;
      for (j = 1; j < i; j++) {
        free (users[j]);
      }
      free (users);
      return 0;
    }
    if (!disable_sql) {
      if (vk_db_insert_user (users[i]) < 0) {
        int j;
        for (j = 1; j <= i; j++) {
          free (users[j]);
        }
        free (users);
        return 0;
      }
    }
  }
  return users;
}

int vk_users_finalize (struct vk_curl_handle *handle UNUSED, void *data) {
  struct user **users = data;
  int n = (long)*users;
  print_user_array (n, users + 1);
  return 0;
}

int aio_profiles_get (int num, const int *ids, int silent) {
  if (!access_token) {
    vk_error (ERROR_NO_ACCESS_TOKEN, "No access token");
    return _ERROR;
  }

  if (num <= 0 || num >= 100) {
    vk_error (ERROR_COMMAND_LINE, "invalid number of users");
    return _ERROR;
  }

  static char query[10001];
  int k = snprintf (query, 10000, "https://api.vk.com/method/getProfiles?uids=");
  int i;
  for (i = 0; i < num; i++) {
    if (i == 0) {
      k += sprintf (query + k, "%d", ids[i]);
    } else {
      k += sprintf (query + k, ",%d", ids[i]);
    }
  }
  k += sprintf (query + k, "&fields=uid,first_name,last_name,nickname,screen_name,sex,bdate,city,country,timezone,photo,photo_medium,photo_big,has_mobile,rate,contacts,education,online,counters,can_post,can_see_all_posts,can_write_private_message,activity,last_seen,relation,wall_comments,relatives&access_token=%s", access_token);
  
  struct vk_methods methods = {
    .parse = default_parse,
    .on_end = vk_users_get_aio,
    .finalize = vk_users_finalize
  };
  if (silent) {
    methods.finalize = 0;
  }
  struct vk_curl_handle *handle = get_handle ();
  if (!handle) { return _ERROR; }
  return do_query (handle, query, &methods, 0, 0);
}

/* }}} */

/* {{{ Wall post methods */
void *vk_wall_post_check_aio (struct vk_curl_handle *handle UNUSED, json_t *ans) {
  if (!(ans = json_object_get (ans, "response"))) {
    vk_error (ERROR_UNEXPECTED_ANSWER, "No response field in answer");
    return 0;
  }
  vk_log (2, "Wall post id is %d\n", (int)json_integer_value (json_object_get (ans, "post_id")));
  return (void *)(-1l);
}

int vk_wall_post_check_finalize (struct vk_curl_handle *handle UNUSED, void *data) {
  assert (data == (void *)(-1l));
  printf ("Successfully sent\n");
  return 0;
}

int aio_wall_post (int id, const char *msg) {
  if (!access_token) {
    vk_error (ERROR_NO_ACCESS_TOKEN, "No access token");
    return _ERROR;
  }
  static char query[10001];
  char *q = curl_easy_escape (curl_handle, msg, 0);
  snprintf (query, 10000, "https://api.vk.com/method/wall.post?owner_id=%d&message=%s&access_token=%s", id, q, access_token);
  curl_free (q);
 
  struct vk_methods methods = {
    .parse = default_parse,
    .on_end = vk_wall_post_check_aio,
    .finalize = vk_wall_post_check_finalize,
    .free = do_nothing_free
  };
  struct vk_curl_handle *handle = get_handle ();
  if (!handle) { return _ERROR; }
  return do_query (handle, query, &methods, 0, 0);
}
/* }}} */

/* {{{ Send message methods */
void *vk_msg_check_send_aio (struct vk_curl_handle *handle UNUSED, json_t *ans) {
  if (!(ans = json_object_get (ans, "response"))) {
    vk_error (ERROR_UNEXPECTED_ANSWER, "No response field in answer");
    return 0;
  }
  vk_log (2, "Message id is %d\n", (int)json_integer_value (ans));
  return (void *)(-1l);
}

int vk_msg_check_finalize (struct vk_curl_handle *handle UNUSED, void *data) {
  assert (data == (void *)(-1l));
  printf ("Successfully sent\n");
  return 0;
}

int aio_msg_send (int id, const char *msg) {
  if (!access_token) {
    vk_error (ERROR_NO_ACCESS_TOKEN, "No access token");
    return _ERROR;
  }
  static char query[10001];
  char *q = curl_easy_escape (curl_handle, msg, 0);
  snprintf (query, 10000, "https://api.vk.com/method/messages.send?uid=%d&message=%s&access_token=%s", id, q, access_token);
  curl_free (q);
 
  struct vk_methods methods = {
    .parse = default_parse,
    .on_end = vk_msg_check_send_aio,
    .finalize = vk_msg_check_finalize,
    .free = do_nothing_free
  };
  struct vk_curl_handle *handle = get_handle ();
  if (!handle) { return _ERROR; }
  return do_query (handle, query, &methods, 0, 0);
}
/* }}} */

/* {{{ Get message methods */
void *vk_msgs_get_aio (struct vk_curl_handle *handle UNUSED, json_t *ans) {
  if (!(ans = json_object_get (ans, "response"))) {
    vk_error (ERROR_UNEXPECTED_ANSWER, "No response field in answer");
    return 0;
  }
  int l = json_array_size (ans);
  struct message **messages = malloc (sizeof (void *) * l);
  int i;
  int cc = 1;
  for (i = 1; i < l; i++) {
    messages[cc] = vk_parse_message (json_array_get (ans, i));
    if (!messages[cc]) {
      int j;
      for (j = 1; j < cc; j++) {
        free (messages[j]);
      }
      free (messages);
      return 0;
    }
    if (!messages[cc]->uid) {
      free (messages[cc]);
    } else {
 
      if (!disable_sql) {
        if (vk_db_insert_message (messages[cc]) < 0) {
          int j;
          for (j = 1; j <= cc; j++) {
            free (messages[j]);
          }
          free (messages);
          return 0;
        }
      }
      cc ++;
    }
  }
  messages[0] = (void *)(long )(cc - 1);
  return messages;
}

int vk_msgs_finalize (struct vk_curl_handle *handle, void *data) {
  struct message **messages = data;
  int n = (long)*messages;
  print_message_array (n, messages + 1, (handle->flags & (1ll << 32)) != 0);
  return 0;
}

int aio_msgs_get (int offset, int limit, int reverse, int out) {
  if (!access_token) {
    vk_error (ERROR_NO_ACCESS_TOKEN, "No access token");
    return _ERROR;
  }

  if (limit <= 0) {
    limit = 100;
  }
  if (limit > 100) {
    limit = 100;
  }
  if (offset < 0) {
    offset = 0;
  }

  static char query[1001];
  snprintf (query, 1000, "https://api.vk.com/method/messages.get?out=%d&offset=%d&count=%d&access_token=%s", out, offset, limit, access_token);
  
  struct vk_methods methods = {
    .parse = default_parse,
    .on_end = vk_msgs_get_aio,
    .finalize = vk_msgs_finalize
  };
  struct vk_curl_handle *handle = get_handle ();
  if (!handle) { return _ERROR; }
  return do_query (handle, query, &methods, reverse ? 1 : 0, 0);
}
/* }}} */

/* {{{ Force update messages methods */

int aio_force_update (void);
int vk_force_update_finalize (struct vk_curl_handle *handle, void *data) {
  struct message **messages = data;
  int n = (long)*messages;
  print_message_array (n, messages + 1, (handle->flags & (1ll << 32)) != 0);
  if (n == 100) { 
    aio_force_update ();
  }
  return 0;
}

int aio_force_update (void) {
  if (!access_token) {
    vk_error (ERROR_NO_ACCESS_TOKEN, "No access token");
    return _ERROR;
  }

  int limit = 100;
  int offset = vk_get_max_mid () + 1;

  static char query[10001];
  int l = sprintf (query, "https://api.vk.com/method/messages.getById?mids=");
  int i;
  for (i = 0; i < limit; i++) {
    l += sprintf (query + l, "%s%d", i ? "," : "", offset + i);
  }
  l += sprintf (query + l, "&access_token=%s", access_token);
  
  struct vk_methods methods = {
    .parse = default_parse,
    .on_end = vk_msgs_get_aio,
    .finalize = vk_force_update_finalize
  };
  struct vk_curl_handle *handle = get_handle ();
  if (!handle) { return _ERROR; }
  return do_query (handle, query, &methods, 0, 0);
}
/* }}} */

/* {{{ Main curl-work loop */
int work_one (struct vk_curl_handle *handle, int result) {
  working_handle_count --;
  int r = curl_multi_remove_handle (multi_handle, handle->handle);
  if (r != CURLE_OK) {
    vk_error (ERROR_CURL, "Curl error %d: %s\n", r, curl_multi_strerror (r));
    handle->flags &= ~1;
    return _FATAL_ERROR;
  }
  if (result != CURLE_OK) {
    if (handle->methods.on_curl_fail) {
      r = handle->methods.on_curl_fail (handle, result);
    } else {
      r = default_on_curl_fail (handle, result);
    }
    handle->flags &= ~1;
    return r;
  }
  if (handle->methods.parse) {
    json_t *ans = handle->methods.parse (handle);
    if (!ans) {
      if (handle->methods.on_parse_fail) {
        r = handle->methods.on_parse_fail (handle);
      } else {
        r = _ERROR;
      }
      handle->flags &= ~1;
      return r;
    }
    void *data = handle->methods.on_end (handle, ans);
    json_decref (ans);
    if (!data) {
      handle->flags &= ~1;
      return _ERROR;
    }
    if (handle->methods.finalize) {
      r = handle->methods.finalize (handle, data);
    } else {
      r = 0;
    }
    if (handle->methods.free) {
      handle->methods.free (handle, data);
    } else {
      default_free (handle, data);
    }
    handle->flags &= ~1;
    return r;
  }
  return 0;
}

int work_read_write (void) {
  int nn;
  int r = curl_multi_perform (multi_handle, &nn);
  assert (nn <= working_handle_count);
  if (r != CURLE_OK) {
    vk_error (ERROR_CURL, "Curl error %d: %s\n", r, curl_multi_strerror (r));
    return _ERROR;
  }
  int n;
  CURLMsg *msg;
  int cc = 0;
  while ((msg = curl_multi_info_read (multi_handle, &n))) {
    if (msg->msg != CURLMSG_DONE) { continue; }
    struct vk_curl_handle *handle = vk_choose_by_handle (msg->easy_handle);
    assert (handle);
    cc ++;
    if (work_one (handle, msg->data.result) == _FATAL_ERROR) {
      return _FATAL_ERROR;
    }
  }
  //assert (nn == working_handle_count);
  return cc;
}
/* }}} */

/* {{{ INIT */
size_t save_to_buff (char *ptr, size_t size, size_t nmemb, void *userdata) {
  long long total_bytes = size * (long long)nmemb;
  struct curl_buf *buf = userdata;
  while (total_bytes + buf->pos >= buf->len) {
    int new_len = 2 * buf->len > total_bytes + buf->pos  + 1 ? 2 * buf->len : total_bytes + buf->pos + 1;
    buf->buf = realloc (buf->buf, new_len);
    assert (buf->buf);
    buf->len = new_len;
  }
  memcpy (buf->buf + buf->pos, ptr, total_bytes);
  buf->pos += total_bytes;
  buf->buf[buf->pos] = 0;
  return total_bytes;
}

int set_curl_options (CURL *curl_handle) {
  static char _curl_errorbuf[CURL_ERROR_SIZE];
  TRY_CURL (curl_easy_setopt (curl_handle, CURLOPT_ERRORBUFFER, _curl_errorbuf));
  TRY_CURL (curl_easy_setopt (curl_handle, CURLOPT_VERBOSE, verbosity >= 1));
  TRY_CURL (curl_easy_setopt (curl_handle, CURLOPT_HEADER, 0));
  TRY_CURL (curl_easy_setopt (curl_handle, CURLOPT_NOPROGRESS, 1));
  TRY_CURL (curl_easy_setopt (curl_handle, CURLOPT_WRITEFUNCTION, save_to_buff));
  return 0;
}

int vk_net_init (int create_multi, int max_parallel_queries) {
  if (curl_global_init (CURL_GLOBAL_ALL)) {
    vk_error (ERROR_CURL_INIT, "Something is very bad with CURL: can not do global_init");
    return _FATAL_ERROR;
  }

  if (create_multi) {
    multi_handle = curl_multi_init ();
    if (!multi_handle) {
      vk_error (ERROR_CURL_INIT, "Something is very bad with CURL: can not do multi_init");
      return _FATAL_ERROR;
    }
    assert (max_parallel_queries >= 2);
    handle_count = max_parallel_queries;
    working_handle_count = 0;
    all_handles = malloc (sizeof (struct vk_curl_handle) * handle_count);
    assert (all_handles);
    memset (all_handles, 0, sizeof (struct vk_curl_handle));
    int i;
    for (i = 0; i < handle_count; i++) {
      all_handles[i].handle = curl_easy_init ();
      if (!all_handles[i].handle) {
        vk_error (ERROR_CURL_INIT, "Something is very bad with CURL: can not do easy_init");
        return _FATAL_ERROR;
      }
      if (set_curl_options (all_handles[i].handle) < 0) {
        return _FATAL_ERROR;
      }
    }
  }
  return 0;
}

CURL *vk_curl_init (void) {
  CURL *curl_handle = curl_easy_init ();
  if (!curl_handle) {
    vk_error (ERROR_CURL_INIT, "Something is very bad with CURL: can not do easy_init");
    return 0;
  }
  
  if (set_curl_options (curl_handle) < 0) {
    curl_easy_cleanup (curl_handle);
    return 0;
  }
  return curl_handle;
}
/* }}} */
