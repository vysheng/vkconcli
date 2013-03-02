#ifndef __QUERY_H__
#define __QUERY_H__

#define QUERY_ABORT -1
#define QUERY_OK 1
#define QUERY_WAIT 0

enum query_state {
  query_state_initialize,
  query_state_lookup_cache,
  query_state_lookup_db,
  query_state_lookup_net,
  query_state_update_db,
  query_state_update_cache,
  query_state_output,
  query_state_done
};

struct query;
struct query_methods {
  int (*lookup_cache)(struct query *);
  int (*lookup_db)(struct query *);
  int (*lookup_net)(struct query *);
  int (*update_db)(struct query *);
  int (*update_cache)(struct query *);
  int (*output)(struct query *);
  //void (*finalize)(struct query *);
  void (*free)(struct query *);
};

struct query {
  enum query_state state;
  struct query_methods *methods;
  void *extra;  
};

void query_work (struct query *q);
struct query *query_create (struct query_methods *methods);
#endif
