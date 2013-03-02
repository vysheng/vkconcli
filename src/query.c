#include "query.h"
#include <malloc.h>
#include <assert.h>

struct query *query_create (struct query_methods *methods) {
  struct query *q = malloc (sizeof (*q));
  q->state = query_state_initialize;
  q->methods = methods;
  q->extra = 0;
  return q;
}

void query_work (struct query *q) {
  while (1) {
    int res;
    switch (q->state) {
    case query_state_initialize:
      q->state = query_state_lookup_cache;
      res = q->methods->lookup_cache (q);
      break;
    case query_state_lookup_cache:
      q->state = query_state_lookup_db;
      res = q->methods->lookup_db (q);
      break;
    case query_state_lookup_db:
      q->state = query_state_lookup_net;
      res = q->methods->lookup_net (q);
      break;
    case query_state_lookup_net:
      q->state = query_state_update_db;
      res = q->methods->update_db (q);
      break;
    case query_state_update_db:
      q->state = query_state_update_cache;
      res = q->methods->update_cache (q);
      break;
    case query_state_update_cache:
      q->state = query_state_output;
      res = q->methods->output (q);
      break;
    case query_state_done:
      res = QUERY_ABORT;
      break;
    default:
      assert (0);
    }
    switch (res) {
    case QUERY_ABORT:
      q->methods->free (q);
      return;
    case QUERY_WAIT:
      return;
    case QUERY_OK:
      break;
    default:
      assert (0);
    }
  }
}
