#ifndef __QUERY_H__
#define __QUERY_H__
enum query_state {
  query_state_initialize,
  query_state_lookup_db,
  query_state_lookup_net,
  query_state_update_db,
  query_state_done
};

typedef int (*query_next_op_t)(struct query *query);
typedef void (*query_delete_t)(struct query *query);

struct query {
  enum query_state state;
  query_next_op_t on_next;
  query_delete_t on_delete;
  void *extra;  
};
#endif
