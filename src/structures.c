#include "structures.h"


#include <jansson.h>
#include <string.h>
#include <assert.h>

int parse_int_value (const json_t *J, const char *f, int def) {
  return json_object_get (J, f) ? json_integer_value (json_object_get (J, f)) : def;
}

char *parse_string_value (const json_t *J, const char *f, const char *def) {
  return json_object_get (J, f) ? strdup (json_string_value (json_object_get (J, f))) : def ? strdup (def) : 0;
}

#define GET_INT_FIELD(J,r,s) r->s = parse_int_value (J, #s, 0)
#define GET_STRING_FIELD(J,r,s) r->s = parse_string_value (J, #s, "")


void print_spaces (int level) {
	int i;
	for (i = 0; i < level; i++) printf (" ");
}



void print_message (int level, const struct message *msg) {
	assert (msg->type == TYPE_MESSAGE);

  print_spaces (level);
  printf ("Message #%d %s user %d\n", msg->mid, msg->out ? "to" : "from", msg->uid);
  print_spaces (level + 1);
  printf ("Created at %d, state %s\n", msg->date, msg->read_state ? "read" : "unread");
  print_spaces (level + 1);
  if (msg->chat_id <= 0) {
    printf ("No chat\n");
  } else {
    printf ("Chat_id %d, users_count %d, admin_id %d\n", msg->chat_id, msg->users_count, msg->admin_id);
  }
  print_spaces (level + 1);
  printf ("Title %s\n", msg->title ? msg->title : "<none>");
  print_spaces (level + 1);
  printf ("Body %s\n", msg->body ? msg->body : "<none>");

}

struct message *parse_message (const json_t *J) {
	struct message *r = malloc (sizeof (struct message));
	memset (r, 0, sizeof (*r));
	r->type = TYPE_MESSAGE;
	
	GET_INT_FIELD(J,r,mid);
	GET_INT_FIELD(J,r,uid);
	GET_INT_FIELD(J,r,date);
	GET_INT_FIELD(J,r,out);
	GET_INT_FIELD(J,r,read_state);

	GET_STRING_FIELD(J,r,body);
	GET_STRING_FIELD(J,r,title);
	
	GET_INT_FIELD(J,r,chat_id);
	GET_INT_FIELD(J,r,users_count);
	GET_INT_FIELD(J,r,admin_id);
	GET_INT_FIELD(J,r,deleted);

	GET_INT_FIELD(J,r,emoji);
	return r;
}
