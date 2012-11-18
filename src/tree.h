#ifndef __TREE_H__
#define __TREE_H__
#include "structures.h"

#define DB_QUERY_BUF_SIZE (1 << 20)
int db_message_insert (struct message *msg);

int vk_db_init (const char *filename);
#endif
