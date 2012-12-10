#ifndef __TREE_H__
#define __TREE_H__
#include "structures.h"

#define DB_QUERY_BUF_SIZE (1 << 20)
int db_message_insert (struct message *msg);

int vk_db_init (const char *filename);
int vk_alias_add (const char *id, const char *result);
char *vk_alias_get (const char *id);
int vk_get_max_mid (void);
#endif
