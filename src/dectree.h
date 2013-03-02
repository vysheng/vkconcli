#ifndef __DECTREE_H__
#define __DECTREE_H__

#include "vkconcli.h"

struct tree {
  struct tree *left, *right;
  struct ID *x;
  int y;
};

struct tree *tree_delete (struct tree *T, struct ID *x) __attribute__ ((warn_unused_result));
struct tree *tree_insert (struct tree *T, struct ID *x, int y) __attribute__ ((warn_unused_result));
struct ID *tree_lookup (struct tree *T, struct ID *x);
#endif
