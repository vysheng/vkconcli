
#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <assert.h>
#include <string.h>

#include "vk_errors.h"
#include "structures-auto.h"

extern int verbosity;
extern sqlite3 *db_handle;


static int parse_integer_value1 (const json_t *J, const char *f) __attribute__ ((unused));
static int parse_integer_value2 (const json_t *J, const char *f1, const char *f2) __attribute__ ((unused));
static char *parse_string_value1 (const json_t *J, const char *f) __attribute__ ((unused));
static char *parse_string_value2 (const json_t *J, const char *f1, const char *f2) __attribute__ ((unused));

static int parse_integer_value1 (const json_t *J, const char *f) {
  return json_object_get (J, f) ? json_integer_value (json_object_get (J, f)) : 0;
}

static int parse_integer_value2 (const json_t *J, const char *f1, const char *f2) {
  json_t *J1;
  if (!(J1 = json_object_get (J, f1))) { return 0; }
  return parse_integer_value1 (J1, f2);
}

static char *parse_string_value1 (const json_t *J, const char *f) {
  return json_object_get (J, f) ? strdup (json_string_value (json_object_get (J, f))) : strdup ("");
}

static char *parse_string_value2 (const json_t *J, const char *f1, const char *f2) {
  json_t *J1;
  if (!(J1 = json_object_get (J, f1))) { return strdup (""); }
  return parse_string_value1 (J1, f2);
}

static int ct_callback (void *_ __attribute__ ((unused)), int argc, char **argv, char **cols) {
  if (verbosity >= 2) {
    int i;
    for (i = 0; i < argc; i++) {
      fprintf (stderr, "%s = %s\n", cols[i], argv[i] ? argv[i] : "NULL");
    }
    fprintf (stderr, "\n");
  }
  return 0;
}

int vk_check_table_users (void) {
  char *q = sqlite3_mprintf ("%s", "CREATE TABLE IF NOT EXISTS users(id INT PRIMARY KEY,first_name VARCHAR,last_name VARCHAR,screen_name VARCHAR,nickname VARCHAR,sex TINY_INT,birth VARCHAR,photo VARCHAR,photo_medium VARCHAR,photo_big VARCHAR,city INT,country INT,timezone INT,has_mobile INT,rate INT,university INT,faculty INT,graduation INT,mobile_phone VARCHAR,home_phone VARCHAR,can_post TINY_INT,can_see_all_posts TINY_INT,can_write_private_message TINY_INT,wall_comments TINY_INT,activity VARCHAR,last_seen INT,relation INT,relation_partner_id INT,albums INT,videos INT,audios INT,notes INT,friends INT,groups INT,mutual_friends INT,user_videos INT,followers INT,user_photos INT,subscriptions INT);");
  assert (q);
  if (verbosity >= 4) {
    fprintf (stderr, "%s\n", q);   
  }
  char *e = 0;

  if (sqlite3_exec (db_handle, q, ct_callback, 0, &e) != SQLITE_OK || e) {
    vk_error (ERROR_SQLITE, "SQLite error %s\n", e);
    return _ERROR;
  }

  sqlite3_free (q);
  
  return 0;
}

int vk_db_insert_user (struct user *r) {
  struct user *old = vk_db_lookup_user (r->id);
  if (old && !cmp_user (r, old)) { return 0; }
  char *q;
  if (!old) {
    q = sqlite3_mprintf ("INSERT INTO users (id,first_name,last_name,screen_name,nickname,sex,birth,photo,photo_medium,photo_big,city,country,timezone,has_mobile,rate,university,faculty,graduation,mobile_phone,home_phone,can_post,can_see_all_posts,can_write_private_message,wall_comments,activity,last_seen,relation,relation_partner_id,albums,videos,audios,notes,friends,groups,mutual_friends,user_videos,followers,user_photos,subscriptions) VALUES (%d,%Q,%Q,%Q,%Q,%d,%Q,%Q,%Q,%Q,%d,%d,%d,%d,%d,%d,%d,%d,%Q,%Q,%d,%d,%d,%d,%Q,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d);",r->id,r->first_name,r->last_name,r->screen_name,r->nickname,r->sex,r->birth,r->photo,r->photo_medium,r->photo_big,r->city,r->country,r->timezone,r->has_mobile,r->rate,r->university,r->faculty,r->graduation,r->mobile_phone,r->home_phone,r->can_post,r->can_see_all_posts,r->can_write_private_message,r->wall_comments,r->activity,r->last_seen,r->relation,r->relation_partner_id,r->albums,r->videos,r->audios,r->notes,r->friends,r->groups,r->mutual_friends,r->user_videos,r->followers,r->user_photos,r->subscriptions);
  } else {
    q = sqlite3_mprintf ("UPDATE users SET id=%d,first_name=%Q,last_name=%Q,screen_name=%Q,nickname=%Q,sex=%d,birth=%Q,photo=%Q,photo_medium=%Q,photo_big=%Q,city=%d,country=%d,timezone=%d,has_mobile=%d,rate=%d,university=%d,faculty=%d,graduation=%d,mobile_phone=%Q,home_phone=%Q,can_post=%d,can_see_all_posts=%d,can_write_private_message=%d,wall_comments=%d,activity=%Q,last_seen=%d,relation=%d,relation_partner_id=%d,albums=%d,videos=%d,audios=%d,notes=%d,friends=%d,groups=%d,mutual_friends=%d,user_videos=%d,followers=%d,user_photos=%d,subscriptions=%d WHERE id=%d;",r->id,r->first_name,r->last_name,r->screen_name,r->nickname,r->sex,r->birth,r->photo,r->photo_medium,r->photo_big,r->city,r->country,r->timezone,r->has_mobile,r->rate,r->university,r->faculty,r->graduation,r->mobile_phone,r->home_phone,r->can_post,r->can_see_all_posts,r->can_write_private_message,r->wall_comments,r->activity,r->last_seen,r->relation,r->relation_partner_id,r->albums,r->videos,r->audios,r->notes,r->friends,r->groups,r->mutual_friends,r->user_videos,r->followers,r->user_photos,r->subscriptions,r->id);
  }
  assert (q);
  if (verbosity >= 4) {
    fprintf (stderr, "%s\n", q);
  }
  char *e = 0;

  if (sqlite3_exec (db_handle, q, ct_callback, 0, &e) != SQLITE_OK || e) {
    vk_error (ERROR_SQLITE, "SQLite error %s\n", e);
    return _ERROR;
  }

  sqlite3_free (q);
  
  return 0;
}

struct user *vk_parse_user (json_t *J) {
  struct user *r = malloc (sizeof (*r));
  r->id = parse_integer_value1 (J,"uid");
  r->first_name = parse_string_value1 (J,"first_name");
  r->last_name = parse_string_value1 (J,"last_name");
  r->screen_name = parse_string_value1 (J,"screen_name");
  r->nickname = parse_string_value1 (J,"nickname");
  r->sex = parse_integer_value1 (J,"sex");
  r->birth = parse_string_value1 (J,"bdate");
  r->photo = parse_string_value1 (J,"photo");
  r->photo_medium = parse_string_value1 (J,"photo_medium");
  r->photo_big = parse_string_value1 (J,"photo_big");
  r->city = parse_integer_value1 (J,"city");
  r->country = parse_integer_value1 (J,"country");
  r->timezone = parse_integer_value1 (J,"timezone");
  r->has_mobile = parse_integer_value1 (J,"has_mobile");
  r->rate = parse_integer_value1 (J,"rate");
  r->university = parse_integer_value1 (J,"university");
  r->faculty = parse_integer_value1 (J,"faculty");
  r->graduation = parse_integer_value1 (J,"graduation");
  r->mobile_phone = parse_string_value1 (J,"mobile_phone");
  r->home_phone = parse_string_value1 (J,"home_phone");
  r->can_post = parse_integer_value1 (J,"can_post");
  r->can_see_all_posts = parse_integer_value1 (J,"can_see_all_posts");
  r->can_write_private_message = parse_integer_value1 (J,"can_write_private_message");
  r->wall_comments = parse_integer_value1 (J,"wall_comments");
  r->activity = parse_string_value1 (J,"activity");
  r->last_seen = parse_integer_value1 (J,"last_seen");
  r->relation = parse_integer_value1 (J,"relation");
  r->relation_partner_id = parse_integer_value2 (J,"relation_partner","id");
  r->albums = parse_integer_value2 (J,"counters","albums");
  r->videos = parse_integer_value2 (J,"counters","videos");
  r->audios = parse_integer_value2 (J,"counters","audios");
  r->notes = parse_integer_value2 (J,"counters","notes");
  r->friends = parse_integer_value2 (J,"counters","friends");
  r->groups = parse_integer_value2 (J,"counters","groups");
  r->mutual_friends = parse_integer_value2 (J,"counters","mutual_friends");
  r->user_videos = parse_integer_value2 (J,"counters","user_videos");
  r->followers = parse_integer_value2 (J,"counters","followers");
  r->user_photos = parse_integer_value2 (J,"counters","user_photos");
  r->subscriptions = parse_integer_value2 (J,"counters","subscriptions");

  return r;
}

struct t_arr_user {
  int cur_num;
  int max_num;
  struct user **r;
};

static int ct_user_callback (void *_r, int argc, char **argv, char **cols) {
  if (verbosity >= 2) {
    int i;
    for (i = 0; i < argc; i++) {
      fprintf (stderr, "%s = %s\n", cols[i], argv[i] ? argv[i] : "NULL");
    }
    fprintf (stderr, "\n");
  }
  struct t_arr_user *a = _r;
  if (a->cur_num == a->max_num) { return 0;}
  struct user *r = malloc (sizeof (*r));
  a->r[a->cur_num ++] = r;
  r->id = argv[0] ? atoi (argv[0]) : 0;
  r->first_name = argv[1] ? strdup (argv[1]) : strdup ("");
  r->last_name = argv[2] ? strdup (argv[2]) : strdup ("");
  r->screen_name = argv[3] ? strdup (argv[3]) : strdup ("");
  r->nickname = argv[4] ? strdup (argv[4]) : strdup ("");
  r->sex = argv[5] ? atoi (argv[5]) : 0;
  r->birth = argv[6] ? strdup (argv[6]) : strdup ("");
  r->photo = argv[7] ? strdup (argv[7]) : strdup ("");
  r->photo_medium = argv[8] ? strdup (argv[8]) : strdup ("");
  r->photo_big = argv[9] ? strdup (argv[9]) : strdup ("");
  r->city = argv[10] ? atoi (argv[10]) : 0;
  r->country = argv[11] ? atoi (argv[11]) : 0;
  r->timezone = argv[12] ? atoi (argv[12]) : 0;
  r->has_mobile = argv[13] ? atoi (argv[13]) : 0;
  r->rate = argv[14] ? atoi (argv[14]) : 0;
  r->university = argv[15] ? atoi (argv[15]) : 0;
  r->faculty = argv[16] ? atoi (argv[16]) : 0;
  r->graduation = argv[17] ? atoi (argv[17]) : 0;
  r->mobile_phone = argv[18] ? strdup (argv[18]) : strdup ("");
  r->home_phone = argv[19] ? strdup (argv[19]) : strdup ("");
  r->can_post = argv[20] ? atoi (argv[20]) : 0;
  r->can_see_all_posts = argv[21] ? atoi (argv[21]) : 0;
  r->can_write_private_message = argv[22] ? atoi (argv[22]) : 0;
  r->wall_comments = argv[23] ? atoi (argv[23]) : 0;
  r->activity = argv[24] ? strdup (argv[24]) : strdup ("");
  r->last_seen = argv[25] ? atoi (argv[25]) : 0;
  r->relation = argv[26] ? atoi (argv[26]) : 0;
  r->relation_partner_id = argv[27] ? atoi (argv[27]) : 0;
  r->albums = argv[28] ? atoi (argv[28]) : 0;
  r->videos = argv[29] ? atoi (argv[29]) : 0;
  r->audios = argv[30] ? atoi (argv[30]) : 0;
  r->notes = argv[31] ? atoi (argv[31]) : 0;
  r->friends = argv[32] ? atoi (argv[32]) : 0;
  r->groups = argv[33] ? atoi (argv[33]) : 0;
  r->mutual_friends = argv[34] ? atoi (argv[34]) : 0;
  r->user_videos = argv[35] ? atoi (argv[35]) : 0;
  r->followers = argv[36] ? atoi (argv[36]) : 0;
  r->user_photos = argv[37] ? atoi (argv[37]) : 0;
  r->subscriptions = argv[38] ? atoi (argv[38]) : 0;

  return 0;
}

struct user *vk_db_lookup_user (int id) {
  struct t_arr_user r;
  r.cur_num = 0;
  r.max_num = 1;
  r.r = malloc (sizeof (void *) * r.max_num);

  char *q = sqlite3_mprintf ("SELECT * FROM users WHERE id = %d LIMIT 1;", id);
  assert (q);
  if (verbosity >= 4) {
    fprintf (stderr, "%s\n", q);
  }
  char *e = 0;

  if (sqlite3_exec (db_handle, q, ct_user_callback, &r, &e) != SQLITE_OK || e) {
    vk_error (ERROR_SQLITE, "SQLite error %s\n", e);
    free (r.r);
    return 0;
  }

  sqlite3_free (q);

  if (r.cur_num == 0) {
    free (r.r);
    return 0;
  } else {
    struct user *t = r.r[0];
    free (r.r);
    return t;
  }
} 

int cmp_user (struct user *a, struct user *b) {
  int x;
  x = a->id - b->id;
  if (x) { return x;}
  x = strcmp (a->first_name, b->first_name);
  if (x) { return x;}
  x = strcmp (a->last_name, b->last_name);
  if (x) { return x;}
  x = strcmp (a->screen_name, b->screen_name);
  if (x) { return x;}
  x = strcmp (a->nickname, b->nickname);
  if (x) { return x;}
  x = a->sex - b->sex;
  if (x) { return x;}
  x = strcmp (a->birth, b->birth);
  if (x) { return x;}
  x = strcmp (a->photo, b->photo);
  if (x) { return x;}
  x = strcmp (a->photo_medium, b->photo_medium);
  if (x) { return x;}
  x = strcmp (a->photo_big, b->photo_big);
  if (x) { return x;}
  x = a->city - b->city;
  if (x) { return x;}
  x = a->country - b->country;
  if (x) { return x;}
  x = a->timezone - b->timezone;
  if (x) { return x;}
  x = a->has_mobile - b->has_mobile;
  if (x) { return x;}
  x = a->rate - b->rate;
  if (x) { return x;}
  x = a->university - b->university;
  if (x) { return x;}
  x = a->faculty - b->faculty;
  if (x) { return x;}
  x = a->graduation - b->graduation;
  if (x) { return x;}
  x = strcmp (a->mobile_phone, b->mobile_phone);
  if (x) { return x;}
  x = strcmp (a->home_phone, b->home_phone);
  if (x) { return x;}
  x = a->can_post - b->can_post;
  if (x) { return x;}
  x = a->can_see_all_posts - b->can_see_all_posts;
  if (x) { return x;}
  x = a->can_write_private_message - b->can_write_private_message;
  if (x) { return x;}
  x = a->wall_comments - b->wall_comments;
  if (x) { return x;}
  x = strcmp (a->activity, b->activity);
  if (x) { return x;}
  x = a->last_seen - b->last_seen;
  if (x) { return x;}
  x = a->relation - b->relation;
  if (x) { return x;}
  x = a->relation_partner_id - b->relation_partner_id;
  if (x) { return x;}
  x = a->albums - b->albums;
  if (x) { return x;}
  x = a->videos - b->videos;
  if (x) { return x;}
  x = a->audios - b->audios;
  if (x) { return x;}
  x = a->notes - b->notes;
  if (x) { return x;}
  x = a->friends - b->friends;
  if (x) { return x;}
  x = a->groups - b->groups;
  if (x) { return x;}
  x = a->mutual_friends - b->mutual_friends;
  if (x) { return x;}
  x = a->user_videos - b->user_videos;
  if (x) { return x;}
  x = a->followers - b->followers;
  if (x) { return x;}
  x = a->user_photos - b->user_photos;
  if (x) { return x;}
  x = a->subscriptions - b->subscriptions;
  if (x) { return x;}

  return 0;
}
