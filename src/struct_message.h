#ifndef __STRUCT_MESSAGE_H__
#define __STRUCT_MESSAGE_H__
#include "vkconcli.h"
#include <jansson.h>
struct message {
  struct object object;
  int uid;
  int date;
  int read_state;
  int out;
  char * title;
  char * body;
  int chat_id;
  int deleted;
  int emoji;
};
int vk_db_insert_message (struct message *r);
struct message *vk_parse_message (json_t *J);
struct message *vk_parse_message_longpoll (json_t *J);
struct message *vk_db_lookup_message (int id);

int vk_check_table_messages (void);
int cmp_message (struct message *a, struct message *b);
#endif
