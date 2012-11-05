#ifndef __STRUCTURES_H__
#define __STRUCTURES_H__

#define TYPE_MESSAGE 0xff0000

#include <jansson.h>

struct ID {
  int type;
  int id;
  int owner;
};

struct message {
  struct ID id;
	int uid;
	int date;
	int read_state;
	int out;
	char *title;
	char *body;
	int attachment_num;
	struct attachement *attachments;
	int forwarded_num;
	int *forwarded;
	int chat_id;
	int chat_active[6];
	int users_count;
	int admin_id;
	int deleted;
	int emoji;
};

struct liked {
	int type;
	int count;
	int me_liked;
	int can_like;
	int can_publish;
};

struct commented {
	int type;
	int count;
	int can_comment;
};

struct reposted {
	int type;
	int count;
	int me_reposted;
};

struct photo {
  struct ID id;
	char *src;
	char *src_big;
};

struct video {
  struct ID id;
	char *title;
	char *description;
	int duration;
	char *preview;
	char *image_big;
	char *image_small;
	int views;
	int date;
};

struct audio {
  struct ID id;
	char *performer;
	char *title;
	int duration;
	char *url;
};

struct document {
  struct ID id;
	char *title;
	int size;
	char *ext;
	char *url;
};

struct post_source {
	int type;
	char *post_type;
	char *data;
};

struct post {
  struct ID id;
	int from_id;
	int date;
	char *text;
	struct liked liked;
	struct commented commented;
	int attachement_num;
	struct attachment *attachment;
	struct geo *geo;
	struct post_source post_source;
	int signer_id;
	int copy_owner_id;
	int copy_post_id;
	char *copy_text;
};

struct place {
  struct ID id;
	double longitude;
	double latitude;
	int country_id;
	int city_id;
	char *title;
	char *address;
};

struct geo {
	int type;
	union {
		struct place place;
	} data;
};

struct graffity {
  struct ID id;
	char *src;
	char *src_big;
};

struct link {
	int type;
	char *url;
	char *title;
	char *description;
	char *image_src;
};

struct note {
  struct ID id;
	char *title;
	int ncom;
};

struct app {
  struct ID id;
	char *name;
	char *src;
	char *src_big;
};

struct poll {
  struct ID id;
	char *question;
};

struct page {
  struct ID id;
	char *title;
};

struct attachment {
	int type;
	union {
		struct photo photo;
		struct video video;
		struct audio audio;
		struct document document;
		struct post post;
		struct graffity graffity;
		struct link link;
		struct note note;
		struct app app;
		struct poll poll;
		struct page page;
	} data;
};

void print_message (int level, const struct message *msg);
struct message *parse_message (const json_t *J);

#endif
