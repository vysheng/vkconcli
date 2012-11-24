#include "structures.h"


#include <jansson.h>
#include <string.h>
#include <assert.h>
#include "tree.h"
#include "net.h"
#include "structures-auto.h"

#include <time.h>
#include <curl/curl.h>

struct message *vk_parse_message_longpoll (json_t *J) {
  struct message *r = malloc (sizeof (*r));
  r->id = json_integer_value (json_array_get (J, 1));
  int flags = json_integer_value (json_array_get (J, 2));
  r->uid = json_integer_value (json_array_get (J, 3));
  r->date = json_integer_value (json_array_get (J, 4));
  r->read_state = !(flags & 1);
  r->out = (flags & 2) != 0;
  r->title = strdup (json_string_value (json_array_get (J, 5)));
  r->body = strdup (json_string_value (json_array_get (J, 6)));
  r->chat_id = (flags & 16) ? -1 : 0;
  r->deleted = (flags & 128) != 0;
  r->emoji = 0;
  return r;
}

void print_spaces (int level) {
	int i;
	for (i = 0; i < level; i++) printf (" ");
}

extern int disable_sql;
extern CURL *curl_handle;



void print_message (int level, const struct message *msg) {
  print_spaces (level);
  struct user *user = 0;
  if (!disable_sql) {
    user = vk_db_lookup_user (msg->uid);
    if (!user) {
      aio_profiles_get (1, &msg->uid, 1);
    }
  }
  if (!user) {
    printf ("Message #%d %s user %d\n", msg->id, msg->out ? "to" : "from", msg->uid);
  } else {
    printf ("Message #%d %s user %d (%s %s)\n", msg->id, msg->out ? "to" : "from", msg->uid, user->first_name, user->last_name);
  }
  struct tm *lctime = localtime ((void *)&(msg->date));
  print_spaces (level + 1);
  printf ("Created at [%d-%d-%d %d:%d:%d], state %s\n", 
    lctime->tm_year + 1900, lctime->tm_mon + 1, lctime->tm_mday, 
    lctime->tm_hour, lctime->tm_min, lctime->tm_sec,
    msg->read_state ? "read" : "unread");
  print_spaces (level + 1);
  if (msg->chat_id <= 0) {
    printf ("No chat\n");
  } else {
    printf ("Chat_id %d\n", msg->chat_id);
  }
  print_spaces (level + 1);
  printf ("Title %s\n", msg->title ? msg->title : "<none>");
  print_spaces (level + 1);
  printf ("Body %s\n", msg->body ? msg->body : "<none>");

}

void print_message_array (int num, struct message **msg, int reverse) {
  int i;
  if (!reverse) {
    for (i = 0; i < num; i++) {
      printf ("----------\n");
      print_message (0, msg[i]);
    }
    printf ("----------\n");
  } else {
    for (i = num - 1; i >= 0; i--) {
      printf ("----------\n");
      print_message (0, msg[i]);
    }
    printf ("----------\n");
  }
}

void print_user (int level, const struct user *user) {
  print_spaces (level);
  printf ("User #%d: %s %s. URL: https://vk.com/%s\n", user->id, user->first_name, user->last_name, user->screen_name);

  print_spaces (level + 1);
  printf ("Birthday %s. Sex %s.\n", user->birth, user->sex == 2 ? "male" : user->sex == 1 ? "female" : "unknown");
  print_spaces (level + 1);
  printf ("Status %s\n", user->activity);
}


void print_user_array (int num, struct user **users) {
  int i;
  for (i = 0; i < num; i++) {
    printf ("----------\n");
    print_user (0, users[i]);
  }
  printf ("----------\n");
}

