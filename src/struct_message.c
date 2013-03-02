#include "struct_message.h"

#include <sqlite3.h>
#include <assert.h>

#include "util_db.h"
#include "vk_errors.h"
#include "util_json.h"

#include <memory.h>

void vk_message_free (struct message *msg) {
  assert (msg->object.id.type == TYPE_MESSAGE);
  if (msg->title) {
    free (msg->title);
  }
  if (msg->body) {
    free (msg->body);
  }
  free (msg);
}

void vk_message_debug_dump (struct message *msg) {
  assert (msg->object.id.type == TYPE_MESSAGE);
  fprintf (stderr, "message %d:\n", msg->object.id.id);
  fprintf (stderr, "  user %d\n", msg->uid);
  fprintf (stderr, "  date %d\n", msg->date);
  fprintf (stderr, "  read %d\n", msg->read_state);
  fprintf (stderr, "  out %d\n", msg->out);
  fprintf (stderr, "  title %16s\n", msg->title);
  fprintf (stderr, "  body %16s\n", msg->body);
  fprintf (stderr, "  chat_id %d\n", msg->chat_id);
  fprintf (stderr, "  deleted %d\n", msg->deleted);
  fprintf (stderr, "  emoji %d\n", msg->emoji);
}

struct object_methods message_methods = {
  .free = (void *)vk_message_free,
  .debug_dump = (void *)vk_message_debug_dump
};


struct message *vk_message_alloc (void) {
  struct message *msg = malloc (sizeof (*msg));
  memset (msg, 0, sizeof (*msg));
  msg->object.id.type = TYPE_MESSAGE;
  msg->object.methods = &message_methods;
  msg->object.ref_cnt = 1;
  return msg;
}

int vk_check_table_messages (void) {
  char *q = sqlite3_mprintf ("%s", "CREATE TABLE IF NOT EXISTS messages(id INT PRIMARY KEY,uid INT,date INT,read_state TINY_INT,out TINY_INT,title VARCHAR,body VARCHAR,chat_id INT,deleted TINY_INT,emoji TINY_INT);");
  assert (q);
  if (verbosity >= 4) {
    fprintf (stderr, "%s\n", q);   
  }
  char *e = 0;

  if (sqlite3_exec (db_handle, q, ct_db_callback, 0, &e) != SQLITE_OK || e) {
    vk_error (ERROR_SQLITE, "SQLite error %s\n", e);
    return _ERROR;
  }

  sqlite3_free (q);
  
  return 0;
}

int vk_db_insert_message (struct message *r) {
  struct message *old = vk_db_lookup_message (r->object.id.id);
  if (old && !cmp_message (r, old)) { 
    DEC_REF (old);
    return 0; 
  }
  if (old) {
    DEC_REF (old);
  }
  char *q;
  if (!old) {
    q = sqlite3_mprintf ("INSERT INTO messages (id,uid,date,read_state,out,title,body,chat_id,deleted,emoji) VALUES (%d,%d,%d,%d,%d,%Q,%Q,%d,%d,%d);",r->object.id.id,r->uid,r->date,r->read_state,r->out,r->title,r->body,r->chat_id,r->deleted,r->emoji);
  } else {
    q = sqlite3_mprintf ("UPDATE messages SET id=%d,uid=%d,date=%d,read_state=%d,out=%d,title=%Q,body=%Q,chat_id=%d,deleted=%d,emoji=%d WHERE id=%d;",r->object.id.id,r->uid,r->date,r->read_state,r->out,r->title,r->body,r->chat_id,r->deleted,r->emoji,r->object.id.id);
  }
  assert (q);
  if (verbosity >= 4) {
    fprintf (stderr, "%s\n", q);
  }
  char *e = 0;

  if (sqlite3_exec (db_handle, q, ct_db_callback, 0, &e) != SQLITE_OK || e) {
    vk_error (ERROR_SQLITE, "SQLite error %s\n", e);
    return _ERROR;
  }

  sqlite3_free (q);
  
  return 0;
}

struct message *vk_parse_message (json_t *J) {
  struct message *r = vk_message_alloc ();  
  r->object.id.id = parse_integer_value1 (J,"mid");
  r->uid = parse_integer_value1 (J,"uid");
  r->date = parse_integer_value1 (J,"date");
  r->read_state = parse_integer_value1 (J,"read_state");
  r->out = parse_integer_value1 (J,"out");
  r->title = parse_string_value1 (J,"title");
  r->body = parse_string_value1 (J,"body");
  r->chat_id = parse_integer_value1 (J,"chat_id");
  r->deleted = parse_integer_value1 (J,"deleted");
  r->emoji = parse_integer_value1 (J,"emoji");

  return r;
}

struct message *vk_parse_message_longpoll (json_t *J) {
  struct message *r = vk_message_alloc ();
  r->object.id.id = json_integer_value (json_array_get (J, 1));
  int flags = json_integer_value (json_array_get (J, 2));
  r->uid = json_integer_value (json_array_get (J, 3));
  r->date = json_integer_value (json_array_get (J, 4));
  r->read_state = !(flags & 1);
  r->out = (flags & 2) != 0;
  r->title = strdup (json_string_value (json_array_get (J, 5)));
  r->body = strdup (json_string_value (json_array_get (J, 6)));
  r->chat_id = (flags & 16) ? -1 : 0;
  r->deleted = (flags & 128) != 0;
  r->emoji = 0;
  json_t *t = json_array_get (J, 7);
  if (t && json_object_get (t, "from") && (flags & 16)) {
    r->chat_id = r->uid - 2000000000;
    r->uid = atoi (json_string_value (json_object_get (t, "from")));
  }
  return r;
}

struct t_arr_message {
  int cur_num;
  int max_num;
  struct message **r;
};

static int ct_message_callback (void *_r, int argc, char **argv, char **cols) {
  if (verbosity >= 2) {
    int i;
    for (i = 0; i < argc; i++) {
      fprintf (stderr, "%s = %s\n", cols[i], argv[i] ? argv[i] : "NULL");
    }
    fprintf (stderr, "\n");
  }
  struct t_arr_message *a = _r;
  if (a->cur_num == a->max_num) { return 0;}
  struct message *r = vk_message_alloc ();  
  a->r[a->cur_num ++] = r;
  r->object.id.id = argv[0] ? atoi (argv[0]) : 0;
  r->uid = argv[1] ? atoi (argv[1]) : 0;
  r->date = argv[2] ? atoi (argv[2]) : 0;
  r->read_state = argv[3] ? atoi (argv[3]) : 0;
  r->out = argv[4] ? atoi (argv[4]) : 0;
  r->title = argv[5] ? strdup (argv[5]) : strdup ("");
  r->body = argv[6] ? strdup (argv[6]) : strdup ("");
  r->chat_id = argv[7] ? atoi (argv[7]) : 0;
  r->deleted = argv[8] ? atoi (argv[8]) : 0;
  r->emoji = argv[9] ? atoi (argv[9]) : 0;

  return 0;
}

struct message *vk_db_lookup_message (int id) {
  struct t_arr_message r;
  r.cur_num = 0;
  r.max_num = 1;
  r.r = malloc (sizeof (void *) * r.max_num);

  char *q = sqlite3_mprintf ("SELECT * FROM messages WHERE id = %d LIMIT 1;", id);
  assert (q);
  if (verbosity >= 4) {
    fprintf (stderr, "%s\n", q);
  }
  char *e = 0;

  if (sqlite3_exec (db_handle, q, ct_message_callback, &r, &e) != SQLITE_OK || e) {
    vk_error (ERROR_SQLITE, "SQLite error %s\n", e);
    free (r.r);
    return 0;
  }

  sqlite3_free (q);

  if (r.cur_num == 0) {
    free (r.r);
    return 0;
  } else {
    struct message *t = r.r[0];
    free (r.r);
    return t;
  }
} 

int cmp_message (struct message *a, struct message *b) {
  int x;
  x = a->object.id.id - b->object.id.id;
  if (x) { return x;}
  x = a->uid - b->uid;
  if (x) { return x;}
  x = a->date - b->date;
  if (x) { return x;}
  x = a->read_state - b->read_state;
  if (x) { return x;}
  x = a->out - b->out;
  if (x) { return x;}
  x = strcmp (a->title, b->title);
  if (x) { return x;}
  x = strcmp (a->body, b->body);
  if (x) { return x;}
  x = a->chat_id - b->chat_id;
  if (x) { return x;}
  x = a->deleted - b->deleted;
  if (x) { return x;}
  x = a->emoji - b->emoji;
  if (x) { return x;}

  return 0;
}
