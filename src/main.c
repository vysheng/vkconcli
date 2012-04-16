#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


#include <curl/curl.h>
#include <jansson.h>


#define CLIENT_ID 2870218
#define CLIENT_ID_STR "2870218"
#define CLIENT_SECRET "R9hl0gmUCVAEqYHOYYtu"

int verbosity;

#define BUF_SIZE (1 << 23)
char buf[BUF_SIZE];
int buf_ptr;
int limit;
int offset;
char *access_token;

char *get_access_token (void) {
  if (!access_token) {
    access_token = getenv ("vk_access_token");
    if (!access_token) {
      exit (6);
    }
    access_token = strdup (access_token);
  }
  return access_token;
}

size_t save_to_buff (char *ptr, size_t size, size_t nmemb, void *userdata) {
  if (buf_ptr + size * nmemb >= BUF_SIZE) {
    return 0;
  }
  memcpy (buf + buf_ptr, ptr, size * nmemb);
  buf_ptr += nmemb * size;
  buf[buf_ptr] = 0;
  return nmemb * size;
}

CURL *curl_handle;

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
    exit (3);
  }
}


void usage_auth (void) {
  printf ("vkconcli auth <username> <password>\n");
  exit (1);
}

int act_auth (char **argv, int argc) {
  if (argc != 2) {
    usage_auth ();
  }
  static char query[1001];
  snprintf (query, 1000, "https://oauth.vk.com/token?grant_type=password&client_id=%d&client_secret=%s&username=%s&password=%s&scope=messages,friends", CLIENT_ID, CLIENT_SECRET, argv[0], argv[1]);
  
  set_curl_url (query);
  
  query_perform ();

  if (verbosity >= 1) {
    printf ("%s\n", buf);
  }

  json_error_t *error = 0;
  json_t *ans = json_loadb (buf, buf_ptr, 0, error);
  if (!ans) {
    exit (4);
  }

  if (verbosity >= 1) {
    printf ("Answer parsed\n");
  }

  if (json_object_get (ans, "error")) {
    exit (5);
  }

  if (!json_object_get (ans, "access_token")) {
    exit (5);
  }

  printf ("%s\n", json_string_value (json_object_get (ans, "access_token")));
  return 0; 
}

void usage_msg_read (void) {
  printf ("vkconcli msg read [in|out]\n");
  exit (1);
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

  static char query[1001];
  if (limit <= 0) {
    limit = 100;
  }
  if (limit > 100) {
    limit = 100;
  }
  if (offset < 0) {
    offset = 0;
  }
  snprintf (query, 1000, "https://api.vk.com/method/messages.get?out=%d&offset=%d&count=%d&access_token=%s", out, offset, limit, get_access_token ());
  set_curl_url (query);
  
  query_perform ();

  if (verbosity >= 1) {
    printf ("%s\n", buf);
  }

  json_error_t *error = 0;
  json_t *ans = json_loadb (buf, buf_ptr, 0, error);
  if (!ans) {
    exit (4);
  }

  if (verbosity >= 1) {
    printf ("Answer parsed\n");
  }

  if (json_object_get (ans, "error")) {
    exit (5);
  }

  /*if (!json_object_get (ans, "access_token")) {
    exit (5);
  }

  printf ("%s\n", json_string_value (json_object_get (ans, "access_token")));*/
  return 0; 
}

void usage_msg (void) {
  printf ("vkconcli msg [read, send]\n");
  exit (1);
}

int act_msg (char **argv, int argc) {
  if (argc <= 0) {
    usage_msg ();
  }
  if (!strcmp (argv[0], "read")) {
    return act_msg_read (argv + 1, argc - 1);
  }
  usage_msg ();
  return 0; 
}


void usage_act (void) {
  printf ("vkconcli <action>. Possible actions are:\n");
  printf ("\tauth\n");
  printf ("\tmsg (message, messages)\n");
  exit (1);
}

int act (const char *str, char **argv, int argc) {
  if (!strcmp (str, "auth")) {
    return act_auth (argv, argc);
  }
  if (!strcmp (str, "msg") || !strcmp (str, "message") || !strcmp (str, "messages")) {
    return act_msg (argv, argc);
  }
  usage_act ();
  return 1;
}

void usage (void) {
  printf ("vkconcli [-v] [-h] <action>\n");
  printf ("\t-v: increase verbosity level by 1\n");
  printf ("\t-h: print this help and exit\n");
  printf ("\t-l: limit. Different things\n");
  printf ("\t-a: specify access token\n");
  exit (1);
}

int main (int argc, char **argv) {
  char c;
  while ((c = getopt (argc, argv, "vhl:o:a:")) != -1) {
    switch (c) {
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
      case 'h':
      default:
        usage ();
    }
  }

  argc -= optind;
  argv += optind;
  
  assert (argc >= 0);
  if (!argc) {
    usage ();
  }


  if (curl_global_init (CURL_GLOBAL_ALL)) {
    exit (2);
  }

  curl_handle = curl_easy_init ();
  if (!curl_handle) {
    exit (2);
  }

  set_curl_options ();
  return act (*argv, argv + 1, argc - 1);
}
