#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


#include <curl/curl.h>
#include <jansson.h>

#include "structures.h"
#include "net.h"
#include "vk_errors.h"


#define MAX_MESSAGE_LEN 1000


int verbosity;

int limit;
int offset;
char *access_token;
int reverse;



void usage_auth (void) {
  printf ("vkconcli auth <username> <password>\n");
  exit (ERROR_COMMAND_LINE);
}

int act_auth (char **argv, int argc) {
  if (argc != 2) {
    usage_auth ();
  }
  json_t *ans = vk_auth (argv[0], argv[1]);
  assert (ans);

  if (json_object_get (ans, "error")) {
    exit (ERROR_UNEXPECTED_ANSWER);
  }

  if (!json_object_get (ans, "access_token")) {
    exit (ERROR_UNEXPECTED_ANSWER);
  }

  printf ("%s\n", json_string_value (json_object_get (ans, "access_token")));
  return 0; 
}

void print_user_id (int uid) {
  printf ("%d", uid);
}

void print_datetime (int date) {
  long x = date;
  struct tm *tm = localtime ((time_t *)&x);
  assert (tm);
  printf ("[%02d.%02d.%04d %02d:%02d:%02d]", tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900, tm->tm_hour, tm->tm_min, tm->tm_sec);
}

void print_text (const char *s) {
  printf ("%s\n", s);
}

/*void print_message (json_t *msg) {
  assert (msg);
  int mid = json_object_get (msg, "mid") ? json_integer_value (json_object_get (msg, "mid")) : -1;
  int date = json_object_get (msg, "date") ? json_integer_value (json_object_get (msg, "date")) : -1;
  int out = json_object_get (msg, "out") ? json_integer_value (json_object_get (msg, "out")) : -1;
  int uid = json_object_get (msg, "uid") ? json_integer_value (json_object_get (msg, "uid")) : -1;
  int unr = json_object_get (msg, "read_state") ? !json_integer_value (json_object_get (msg, "read_state")) : -1;
  const char *title = json_object_get (msg, "title") ? json_string_value (json_object_get (msg, "title")) : "";
  const char *body = json_object_get (msg, "body") ? json_string_value (json_object_get (msg, "body")) : "";
  //printf ("Message #%d: message %s " USER_ID_STR " at (%d)\n%s\n%s\n", mid, out ? "to" : "from", USER_ID(uid), date, title, body);
  printf ("Message #%d: message %s ", mid, out ? "to" : "from");
  print_user_id (uid);
  printf (". Sent at ");
  print_datetime (date);
  printf (". State %s.\n", unr ? "unread" : "read");
  print_text (title);
  print_text (body);
}*/

void print_messages (json_t *arr) {
  assert (arr);
  int l = json_array_size (arr);
  printf ("Got %d messages\n", l ? l - 1 : 0);
  int i;
  if (!reverse) {
    for (i = 1; i < l; i++) {
      if (i != 1) {
        printf ("\n");
        printf ("---------\n");
        printf ("\n");
      }
      print_message (0, parse_message (json_array_get (arr, i)));
      //print_message (json_array_get (arr, i));
    }
  } else {
    for (i = l - 1; i >= 1; i--) {
      if (i != l - 1) {
        printf ("\n");
        printf ("---------\n");
        printf ("\n");
      }
      print_message (0, parse_message (json_array_get (arr, i)));
      //print_message (json_array_get (arr, i));
    }
  }
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

  json_t *ans = vk_msgs_get (out, limit, offset);
  assert (ans);
  
  print_messages (json_object_get (ans, "response"));
  return 0; 
}

void usage_msg_send (void) {
  printf ("vkconcli msg send <recipient_id>\n");
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
  json_t *ans = vk_msg_send (atoi (argv[0]), read_msg ());
  return 0; 
}

void usage_msg (void) {
  printf ("vkconcli msg [read | send]\n");
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


void usage_act (void) {
  printf ("vkconcli <action>. Possible actions are:\n");
  printf ("\tauth\n");
  printf ("\tmsg (message, messages)\n");
  exit (ERROR_COMMAND_LINE);
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
  printf ("\t-o: offset. Different things\n");
  printf ("\t-a: specify access token\n");
  printf ("\t-R: print in reverse order\n");
  exit (ERROR_COMMAND_LINE);
}

int main (int argc, char **argv) {
  char c;
  while ((c = getopt (argc, argv, "vhl:o:a:R")) != -1) {
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
      case 'R':
        reverse ++;
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

  assert (vk_net_init () >= 0);
  return act (*argv, argv + 1, argc - 1);
}
