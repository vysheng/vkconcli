#ifndef __VKCONCLI_H__
#define __VKCONCLI_H__
struct ID {
  int type;
  int owner;
  int place;
  int id;
};

#define ID_cmp(a,b) memcmp (a,b,sizeof (struct ID))

struct object;

struct object_methods {
  void (*free)(struct object *);
  void (*debug_dump)(struct object *);
};

struct object {
  struct ID id;
  int ref_cnt;
  struct object_methods *methods;
  struct object *next_cache, *prev_cache;
  int extra[0];
};

#define DEC_REF(x) assert (((struct object *)(x))->ref_cnt -- > 0); if (!((struct object *)(x))->ref_cnt) { ((struct object *)(x))->methods->free ((struct object *)(x)); }
#define INC_REF(x) assert (((struct object *)(x))->ref_cnt ++ > 0);


#define rnd() lrand48 ();

extern int verbosity;

#define TYPE_MESSAGE -1

#endif
