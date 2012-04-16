#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


#include <curl/curl.h>


#define CLIENT_ID 2870218
#define CLIENT_ID_STR "2870218"
#define CLIENT_SECRET "R9hl0gmUCVAEqYHOYYtu"

int verbosity;

#define BUF_SIZE (1 << 23)
char buf[BUF_SIZE];
int buf_ptr;

size_t save_to_buff (char *ptr, size_t size, size_t nmemb, void *userdata) {
  if (buf_ptr + size * nmemb > BUF_SIZE) {
    return 0;
  }
  memcpy (buf + buf_ptr, ptr, size * nmemb);
  buf_ptr += nmemb * size;
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
  snprintf (query, 1000, "https://oauth.vk.com/token?grant_type=password&client_id=%d&client_secret=%s&username=%s&password=%s", CLIENT_ID, CLIENT_SECRET, argv[0], argv[1]);
  set_curl_url (query);
  
  query_perform ();

  if (verbosity) {
    buf[buf_ptr ++] = 0;
    printf ("%s\n", buf);
  }
  return 0; 
}


void usage_act (void) {
  printf ("vkconcli <action>. Possible actions are:\n");
  printf ("\tauth\n");
  exit (1);
}

int act (const char *str, char **argv, int argc) {
  if (!strcmp (str, "auth")) {
    return act_auth (argv, argc);
  }
  usage_act ();
  return 1;
}

void usage (void) {
  printf ("vkconcli [-v] [-h] <action>\n");
  printf ("\t-v: increase verbosity level by 1\n");
  printf ("\t-h: print this help and exit\n");
  exit (1);
}

int main (int argc, char **argv) {
  char c;
  while ((c = getopt (argc, argv, "vh")) != -1) {
    switch (c) {
      case 'v': 
        verbosity ++;
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
