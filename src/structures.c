#include "structures.h"


#include <jansson.h>
#include <string.h>
#include <assert.h>
#include "tree.h"
#include "net.h"
#include "structures-auto.h"
#include "vk_errors.h"
#include "util_io.h"

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
  json_t *t = json_array_get (J, 7);
  if (t && json_object_get (t, "from") && (flags & 16)) {
    r->chat_id = r->uid - 2000000000;
    r->uid = atoi (json_string_value (json_object_get (t, "from")));
  }
  return r;
}

int line_pos;
#define LINE_SIZE 100000
static char line[LINE_SIZE + 1];
void begin_line (void) {
  line_pos = 0;
}

void end_line (void) {
  line[line_pos] = 0;
  output_string (line);
  line_pos = 0;
}

void add_string_line (char *c, int len) {
  while (len > 0) {
    int x = len > LINE_SIZE - line_pos ? LINE_SIZE - line_pos : len;
    memcpy (line + line_pos, c, x);
    line_pos += x;
    c += x;
    len -= x;
    if (line_pos == LINE_SIZE) {
      end_line ();
      begin_line ();
    }
  }
}

void add_char_line (char c) {
  add_string_line (&c, 1);
}

void print_spaces (int level) {
	int i;
	for (i = 0; i < level; i++) add_char_line (' ');
}

extern int disable_sql;
extern CURL *curl_handle;

void print_message_text (int level, const char *text) {
  if (level >= 0) {
    begin_line ();
    print_spaces (level);
  }
  while (*text) {
    if (!strncmp (text, "<br>", 4)) {
      end_line ();
      begin_line ();
      print_spaces (level);
      text += 4;
    } else if (!strncmp (text, "&lt;", 4)) {
      add_char_line ('<');
      text += 4;
    } else if (!strncmp (text, "&gt;", 4)) {
      add_char_line ('>');
      text += 4;
    } else {
      add_char_line (*(text ++));
    }
  }
  if (line_pos) {
    end_line ();
  }
}

int message_has_line_break (const char *text) {
  if (!text) { return 0; }
  while (*text) {
    if (!strncmp (text, "<br>", 4)) {
      return 1;
    } else {
      text ++;
    }
  }
  return 0;
}

void print_message (int level, const struct message *msg) {
  begin_line ();
  print_spaces (level);
  struct user *user = 0;
  if (!disable_sql) {
    user = vk_db_lookup_user (msg->uid);
    if (!user) {
      aio_profiles_get (1, &msg->uid, 1);
    }
  }
  static char buf[1001];
  int len;
  /*
  if (!user) {  
    len = snprintf (buf, 1000, "Message #%d %s user %d", msg->id, msg->out ? "to" : "from", msg->uid);
  } else {
    len = snprintf (buf, 1000, "Message #%d %s user %d (%s %s)", msg->id, msg->out ? "to" : "from", msg->uid, user->first_name, user->last_name);
  }*/
  if (msg->out) {
    len = snprintf (buf, 1000, COLOR_RED);
  } else {
    len = snprintf (buf, 1000, COLOR_GREEN);
  }
  if (msg->chat_id) {
    len += snprintf (buf + len, 1000 - len, "Chat %s: ", msg->title);
    if (len >= 1000) { len = 1000; }
  }
  if (!user) {
    len += snprintf (buf + len, 1000 - len, "id%d ", msg->uid);
  } else {
    len += snprintf (buf + len, 1000 - len, "%s %s ", user->first_name, user->last_name);
  }
  add_string_line (buf, len);
  //end_line ();
  //begin_line ();

  int now = time (0);

  long _t = msg->date;
  struct tm *lctime = localtime ((void *)&_t);
  print_spaces (level + 1);

  if (now - msg->date <= 60 * 60 * 12) {
    len = snprintf (buf, 1000, "%02d:%02d ", lctime->tm_hour, lctime->tm_min);
  } else if (now - msg->date <= 60 * 60 * 24 * 30) {
    len = snprintf (buf, 1000, "%02d-%02d %02d:%02d ", lctime->tm_mon + 1, lctime->tm_mday, lctime->tm_hour, lctime->tm_min);
  } else {
    len = snprintf (buf, 1000, "%04d-%02d-%02d %02d:%02d ", lctime->tm_year + 1900, lctime->tm_mon + 1, lctime->tm_mday, lctime->tm_hour, lctime->tm_min);
  }
  /*len = snprintf (buf, 1000, "Created at [%04d-%02d-%02d %02d:%02d:%02d], state %s", 
    lctime->tm_year + 1900, lctime->tm_mon + 1, lctime->tm_mday, 
    lctime->tm_hour, lctime->tm_min, lctime->tm_sec,
    msg->deleted ? "deleted" : msg->read_state ? "read" : "unread");*/
  add_string_line (buf, len);
  
  len = snprintf (buf, 1000, "%s " COLOR_NORMAL, msg->out ? "<<<" : ">>>");
  add_string_line (buf, len);
  if (message_has_line_break (msg->body)) {
    end_line ();
    begin_line ();
    print_message_text (level + 1, msg->body ? msg->body : "<none>");
  } else {
    print_message_text (-1, msg->body ? msg->body : "<none>");
    if (line_pos) {
      end_line ();
    }
  }
  /*begin_line ();
  print_spaces (level + 1);
  if (msg->chat_id <= 0) {
    len = snprintf (buf, 1000, "No chat");
  } else {
    len = snprintf (buf, 1000, "Chat_id %d", msg->chat_id);
  }
  add_string_line (buf, len);
  end_line ();*/
}

void print_message_array (int num, struct message **msg, int reverse) {
  int i;
  if (!reverse) {
    for (i = 0; i < num; i++) {
      begin_line ();
      add_string_line ("----------", 10);
      end_line ();
      print_message (0, msg[i]);
    }
    begin_line ();
    add_string_line ("----------", 10);
    end_line ();
  } else {
    for (i = num - 1; i >= 0; i--) {
      begin_line ();
      add_string_line ("----------", 10);
      end_line ();
      print_message (0, msg[i]);
    }
    begin_line ();
    add_string_line ("----------", 10);
    end_line ();
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

