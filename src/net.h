#ifndef __NET_H__
#define __NET_H__
#include <jansson.h>
json_t *vk_auth (const char *user, const char *password);
json_t *vk_msg_send (int id, const char *msg);
json_t *vk_wall_post (int id, const char *msg);
int vk_net_init (int create_multi, int handle_count);
int aio_msgs_get (int offset, int limit, int reverse, int out);
int aio_profiles_get (int num, int *ids);
int aio_msg_send (int id, const char *msg);
int aio_wall_post (int id, const char *msg);
int aio_auth (const char *username, const char *password);
int work_read_write (void);
#endif
