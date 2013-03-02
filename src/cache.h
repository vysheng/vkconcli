#ifndef __CACHE_H__
#define __CACHE_H__
#include "vkconcli.h"

// force = 0  - touch on conflict
// force = 1  - update on conflict
// force = -1 - fail on conflict
int cache_object (struct object *object, int force);
struct object *cache_lookup (struct ID *id);
int cache_delete (struct ID *id);
int cache_touch (struct ID *id);
#endif
