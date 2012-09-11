#include "structures.h"


#include <jansson.h>
#include <string.h>

int parse_int_value (const json_t *J, const char *f, int def) {
  return json_object_get (J, f) ? json_integer_value (json_object_get (J, f)) : def;
}

char *parse_string_value (const json_t *J, const char *f, const char *def) {
  return json_object_get (J, f) ? strdup (json_string_value (json_object_get (J, f))) : def ? strdup (def) : 0;
}

#define GET_INT_FIELD(J,r,s) r->s = parse_int_value (J, #s, 0)
#define GET_STRING_FIELD(J,r,s) r->s = parse_string_value (J, #s, "")


void print_message (const struct message *msg, int level) {
	assert (msg->type == TYPE_MESSAGE);
	for (i = 0; i < level; i++) print (' ');
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
