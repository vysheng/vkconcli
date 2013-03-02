#include "dectree.h"
#include <memory.h>
#include <malloc.h>
#include <assert.h>

struct tree *new_tree_node (struct ID *x, int y) {
  struct tree *T = malloc (sizeof (*T));
  T->x = x;
  T->y = y;
  T->left = T->right = 0;
  return T;
}

void delete_tree_node (struct tree *T) {
  free (T);
}

void tree_split (struct tree *T, struct ID *x, struct tree **L, struct tree **R) {
  if (!T) {
    *L = *R = 0;
  } else {
    int c = ID_cmp (x, T->x);
    if (c < 0) {
      tree_split (T->left, x, L, &T->left);
      *R = T;
    } else {
      tree_split (T->right, x, &T->right, R);
      *L = T;
    }
  }
}

struct tree *tree_insert (struct tree *T, struct ID *x, int y) {
  if (!T) {
    return new_tree_node (x, y);
  } else {
    if (y > T->y) {
      struct tree *N = new_tree_node (x, y);
      tree_split (T, x, &N->left, &N->right);
      return N;
    } else {
      int c = ID_cmp (x, T->x);
      assert (!c);
      return tree_insert (c < 0 ? T->left : T->right, x, y);
    }
  }
}

struct tree *tree_merge (struct tree *L, struct tree *R) {
  if (!L || !R) {
    return L ? L : R;
  } else {
    if (L->y > R->y) {
      R->left = tree_merge (L, R->left);
      return R;
    } else {
      L->right = tree_merge (L->right, R);
      return L;
    }
  }
}

struct tree *tree_delete (struct tree *T, struct ID *x) {
  assert (T);
  int c = ID_cmp (x, T->x);
  if (!c) {
    struct tree *N = tree_merge (T->left, T->right);
    delete_tree_node (T);
    return N;
  } else {
    return tree_delete (c < 0 ? T->left : T->right, x);
  }
}

struct ID *tree_lookup (struct tree *T, struct ID *x) {
  int c;
  while (T && (c = ID_cmp (x, T->x))) {
    T = (c < 0 ? T->left : T->right);
  }
  return T ? T->x : 0;
}
