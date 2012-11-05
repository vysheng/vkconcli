#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


#include <curl/curl.h>
#include <jansson.h>

#include "vk_errors.h"
#define CLIENT_ID 2870218
#define CLIENT_ID_STR "2870218"
#define CLIENT_SECRET "R9hl0gmUCVAEqYHOYYtu"

CURL *curl_handle;
#define BUF_SIZE (1 << 23)
char buf[BUF_SIZE];
int buf_ptr;
extern int verbosity;
char *access_token;

char *get_access_token (void) {
  if (!access_token) {
    access_token = getenv ("vk_access_token");
    if (!access_token) {
      exit (ERROR_NO_ACCESS_TOKEN);
    }
    access_token = strdup (access_token);    
  }
  return access_token;
}

size_t save_to_buff (char *ptr, size_t size, size_t nmemb, void *userdata __attribute__ ((unused)) ) {
  if (buf_ptr + size * nmemb >= BUF_SIZE) {
    return 0;
  }
  memcpy (buf + buf_ptr, ptr, size * nmemb);
  buf_ptr += nmemb * size;
  buf[buf_ptr] = 0;
  return nmemb * size;
}

void set_curl_options (void) {
  curl_easy_setopt (curl_handle, CURLOPT_VERBOSE, verbosity >= 1);
  curl_easy_setopt (curl_handle, CURLOPT_HEADER, 0);
  curl_easy_setopt (curl_handle, CURLOPT_NOPROGRESS, 1);
  curl_easy_setopt (curl_handle, CURLOPT_WRITEFUNCTION, save_to_buff);
}

void set_curl_url (const char *query) {
  curl_easy_setopt (curl_handle, CURLOPT_URL, query);
}

void query_perform (void) {
  int ec = curl_easy_perform (curl_handle);
  if (ec) {
    exit (ERROR_NET);
  }
}

json_t *vk_auth (const char *user, const char *password) {
  assert (strlen (user) <= 100);
  assert (strlen (password) <= 100);
  static char query[1001];
  snprintf (query, 1000, "https://oauth.vk.com/token?grant_type=password&client_id=%d&client_secret=%s&username=%s&password=%s&scope=messages,friends", CLIENT_ID, CLIENT_SECRET, user, password);
  
  set_curl_url (query);
  
  query_perform ();

  if (verbosity >= 2) {
    printf ("%s\n", buf);
  }

  json_error_t *error = 0;
  json_t *ans = json_loadb (buf, buf_ptr, 0, error);
  if (!ans) {
    exit (ERROR_PARSE_ANSWER);
  }

  if (verbosity >= 1) {
    printf ("Answer parsed\n");
  }

  if (json_object_get (ans, "error")) {
    exit (ERROR_UNEXPECTED_ANSWER);
    return 0;
  }

  if (!json_object_get (ans, "access_token")) {
    exit (ERROR_UNEXPECTED_ANSWER);
    return 0;
  }

  return ans;
}

json_t *vk_msgs_get (int out, int offset, int limit) {
  assert (get_access_token ());

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
  snprintf (query, 1000, "https://api.vk.com/method/messages.get?out=%d&offset=%d&count=%d&access_token=%s", out, offset, limit, get_access_token ());
  set_curl_url (query);
  
  query_perform ();

  if (verbosity >= 2) {
    printf ("%s\n", buf);
  }

  json_error_t *error = 0;
  json_t *ans = json_loadb (buf, buf_ptr, 0, error);
  if (!ans) {
    exit (ERROR_PARSE_ANSWER);
  }

  if (verbosity >= 2) {
    printf ("Answer parsed\n");
  }

  if (json_object_get (ans, "error")) {
    exit (ERROR_UNEXPECTED_ANSWER);
  }

  if (!json_object_get (ans, "response")) {
    exit (ERROR_UNEXPECTED_ANSWER);
  }

  return ans;
}

json_t *vk_msg_send (int id, const char *msg) {
  assert (get_access_token ());
  static char query[10001];
  char *q = curl_easy_escape (curl_handle, msg, 0);
  snprintf (query, 10000, "https://api.vk.com/method/messages.send?uid=%d&message=%s&access_token=%s", id, q, get_access_token ());
  //printf ("%s\n", query);
  curl_free (q);
  set_curl_url (query);
  
  query_perform ();

  if (verbosity >= 2) {
    printf ("%s\n", buf);
  }

  json_error_t *error = 0;
  json_t *ans = json_loadb (buf, buf_ptr, 0, error);
  if (!ans) {
    exit (ERROR_PARSE_ANSWER);
  }

  if (verbosity >= 2) {
    printf ("Answer parsed\n");
  }

  if (json_object_get (ans, "error")) {
    exit (ERROR_UNEXPECTED_ANSWER);
  }

  if (!json_object_get (ans, "response")) {
    exit (ERROR_UNEXPECTED_ANSWER);
  }

  return ans; 
}

int vk_net_init (void) {
  if (curl_global_init (CURL_GLOBAL_ALL)) {
    exit (ERROR_CURL_INIT);
    return -1;
  }

  curl_handle = curl_easy_init ();
  if (!curl_handle) {
    exit (ERROR_CURL_INIT);
    return -1;
  }

  set_curl_options ();
  return 0;
}
