
#ifndef __AUTOGENERATED_H__
#define __AUTOGENERATED_H__

#include <jansson.h>
struct message {
  int id;
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
int vk_check_table_messages (void);
int vk_db_insert_message (struct message *r);
struct message *vk_parse_message (json_t *J);
struct message *vk_db_lookup_message (int id);
int cmp_message (struct message *a, struct message *b);
struct user {
  int id;
  char * first_name;
  char * last_name;
  char * screen_name;
  char * nickname;
  int sex;
  char * birth;
  char * photo;
  char * photo_medium;
  char * photo_big;
  int city;
  int country;
  int timezone;
  int has_mobile;
  int rate;
  int university;
  int faculty;
  int graduation;
  char * mobile_phone;
  char * home_phone;
  int can_post;
  int can_see_all_posts;
  int can_write_private_message;
  int wall_comments;
  char * activity;
  int last_seen;
  int relation;
  int relation_partner_id;
  int albums;
  int videos;
  int audios;
  int notes;
  int friends;
  int groups;
  int mutual_friends;
  int user_videos;
  int followers;
  int user_photos;
  int subscriptions;
};
int vk_check_table_users (void);
int vk_db_insert_user (struct user *r);
struct user *vk_parse_user (json_t *J);
struct user *vk_db_lookup_user (int id);
int cmp_user (struct user *a, struct user *b);
#endif