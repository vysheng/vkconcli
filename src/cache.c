#include "cache.h"
#include "dectree.h"
#include "vkconcli.h"
#include <assert.h>
#include <stdlib.h>
#include "util_config.h"
struct tree *cache;
struct object _CacheQ = {.prev_cache = &_CacheQ, .next_cache = &_CacheQ};
struct object *cacheQ = &_CacheQ;

int cached_items;

void del_use (struct object *object) {
  assert (object->prev_cache);
  assert (object->next_cache);
  object->prev_cache->next_cache = object->next_cache;
  object->next_cache->prev_cache = object->prev_cache;
  object->next_cache = object->prev_cache = 0;
  cached_items --;
}

void add_use (struct object *object) {
  assert (!object->prev_cache);
  assert (!object->next_cache);
  object->next_cache = cacheQ->next_cache;
  object->prev_cache = cacheQ;
  object->next_cache->prev_cache = object;
  object->prev_cache->next_cache = object;
  cached_items ++;

  if (cached_items > max_cached_items) {
    assert (cache_delete (&cacheQ->prev_cache->id) > 0);
  }
}

void update_use (struct object *object) {
  del_use (object);
  add_use (object);
}

int cache_object (struct object *object, int force) {
  if (tree_lookup (cache, (struct ID *)object)) {
    switch (force) {
    case 0:
      cache_touch (&object->id);
      return 0;
    case 1:
      assert (cache_delete (&object->id) > 0);
      break;
    default:
      assert (0);
    }
  }
  cache = tree_insert (cache, (struct ID *)object, lrand48 ());
  INC_REF (object);
  add_use (object);
  return 1;
}

int cache_touch (struct ID *id) {
  struct object *object;
  if (!(object = cache_lookup (id))) {
    return 0;
  }
  update_use (object);
  return 1;
}

struct object *cache_lookup (struct ID *id) {
  return (struct object *)tree_lookup (cache, id);
}

int cache_delete (struct ID *id) {
  struct object *object = (struct object *)tree_lookup (cache, id);
  if (!object) {
    return 0;
  } else {
    del_use (object);
    cache = tree_delete (cache, id);
    DEC_REF (object);
    return 1;
  }
}
