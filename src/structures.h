#ifndef __STRUCTURES_H__
#define __STRUCTURES_H__

#include <jansson.h>
#include "structures-auto.h"


void print_message (int level, const struct message *msg);
void print_user (int level, const struct user *user);

#endif
