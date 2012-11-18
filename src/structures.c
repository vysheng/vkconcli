#include "structures.h"


#include <jansson.h>
#include <string.h>
#include <assert.h>
#include "tree.h"

#include "structures-auto.h"


void print_spaces (int level) {
	int i;
	for (i = 0; i < level; i++) printf (" ");
}



void print_message (int level, const struct message *msg) {
  print_spaces (level);
  printf ("Message #%d %s user %d\n", msg->id, msg->out ? "to" : "from", msg->uid);
  print_spaces (level + 1);
  printf ("Created at %d, state %s\n", msg->date, msg->read_state ? "read" : "unread");
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


void print_user (int level, const struct user *user) {
  print_spaces (level);
  printf ("User #%d: %s %s. URL: https://vk.com/%s\n", user->id, user->first_name, user->last_name, user->screen_name);

  print_spaces (level);
  printf ("  Birthday %s. Sex %s.\n", user->birth, user->sex == 2 ? "male" : user->sex == 1 ? "female" : "unknown");
}


