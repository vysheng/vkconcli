#!/usr/bin/python

import sys, pprint

def create_table (table_name, fields):
  global h_file
  global c_file
  s = """struct """ + table_name["c_name"] + """ {\n"""
  for f in fields:
    s = s + "  " + f["c_type"] + " " + f["c_name"] + ";\n"
  s = s + "};\n"

  h_file.write (s)

  s = "CREATE TABLE IF NOT EXISTS " + table_name["sql_name"] + "("
  ok = 0
  for f in fields:
    if ok == 0:
      ok = 1
    else:
      s += ","
    s = s + f["sql_name"] + " " + f["sql_type"]
    if (f["flags"] & 1) == 1:
      s += " PRIMARY KEY"
  s += ");"
  
  h_file.write ("int vk_check_table_" + table_name["sql_name"] + " (void);\n")
  c_file.write ("""
int vk_check_table_""" + table_name["sql_name"] + """ (void) {
  char *q = sqlite3_mprintf (\"%s\", \"""" + s + """\");
  assert (q);
  if (verbosity >= 4) {
    fprintf (stderr, \"%s\\n\", q);   
  }
  char *e = 0;

  if (sqlite3_exec (db_handle, q, ct_callback, 0, &e) != SQLITE_OK || e) {
    vk_error (ERROR_SQLITE, \"SQLite error %s\\n\", e);
    return _ERROR;
  }

  sqlite3_free (q);
  
  return 0;
}
""")

  s = "INSERT INTO " + table_name["sql_name"] + " ("
  ok = 0
  for f in fields:
    if ok == 0:
      ok = 1
    else:
      s += ","
    s = s + f["sql_name"]
  s += ") VALUES ("
  ok = 0
  for f in fields:
    if ok == 0:
      ok = 1
    else:
      s += ","
    if f["sql_type"] == "TINY_INT" or f["sql_type"] == "INT":
      s = s + "%d"
    elif f["sql_type"].startswith ("VARCHAR"):
      s = s + "%Q"
    else:
      assert (0)
  s += ");"

  u = "UPDATE " + table_name["sql_name"] + " SET "
  v = ""
  ok = 0
  for f in fields:
    if ok == 0:
      ok = 1
    else:
      u += ","
      v += ","
    u = u + f["sql_name"] + "="
    v += "r->" + f["c_name"]
    if f["sql_type"] == "TINY_INT" or f["sql_type"] == "INT":
      u = u + "%d"
    elif f["sql_type"].startswith ("VARCHAR"):
      u = u + "%Q"
    else:
      assert (0)
  u += " WHERE id=%d;"
      
  ok = 0;
  t = ""
  for f in fields:
    if ok == 0:
      ok = 1
    else:
      t += ","
    t = t + "r->" + f["c_name"]

  h_file.write ("int vk_db_insert_" + table_name["c_name"] + " (struct " + table_name["c_name"] + " *r);\n")
  c_file.write ("""
int vk_db_insert_""" + table_name["c_name"] + """ (struct """ + table_name["c_name"] + """ *r) {
  struct """ + table_name["c_name"] + """ *old = vk_db_lookup_""" + table_name["c_name"] + """ (r->id);
  if (old && !cmp_""" + table_name["c_name"] + """ (r, old)) { return 0; }
  char *q;
  if (!old) {
    q = sqlite3_mprintf (\"""" + s + """\","""  + t + """);
  } else {
    q = sqlite3_mprintf (\"""" + u + """\",""" + v + """,r->id);
  }
  assert (q);
  if (verbosity >= 4) {
    fprintf (stderr, \"%s\\n\", q);
  }
  char *e = 0;

  if (sqlite3_exec (db_handle, q, ct_callback, 0, &e) != SQLITE_OK || e) {
    vk_error (ERROR_SQLITE, \"SQLite error %s\\n\", e);
    return _ERROR;
  }

  sqlite3_free (q);
  
  return 0;
}
""")

  s = ""
  for f in fields:
    q = f["json_name"].split ("/")
    s = s + "  r->" + f["c_name"] + " = parse_" + f["json_type"] + "_value" + str (len (q)) + " (J"
    for qq in q:
      s = s + ",\"" + qq + "\""
    s = s + ");\n"

  h_file.write ("""struct """ + table_name["c_name"] + """ *vk_parse_""" + table_name["c_name"] + """ (json_t *J);\n""")
  c_file.write ("""
struct """ + table_name["c_name"] + """ *vk_parse_""" + table_name["c_name"] + """ (json_t *J) {
  struct """ + table_name["c_name"] + """ *r = malloc (sizeof (*r));
""" + s + """
  return r;
}
""")


  s = "";
  i = 0
  for f in fields:
    if (f["c_type"] == "int"):
      s += "  r->" + f["c_name"] + " = argv[" + str (i) + "] ? atoi (argv[" + str (i) + "]) : 0;\n"
    elif f["c_type"] == "char *":
      s += "  r->" + f["c_name"] + " = argv[" + str (i) + "] ? strdup (argv[" + str (i) + "]) : strdup (\"\");\n"
    i = i + 1

  c_file.write ("""
struct t_arr_""" + table_name["c_name"] + """ {
  int cur_num;
  int max_num;
  struct """ + table_name["c_name"] + """ **r;
};

static int ct_""" + table_name["c_name"] + """_callback (void *_r, int argc, char **argv, char **cols) {
  if (verbosity >= 2) {
    int i;
    for (i = 0; i < argc; i++) {
      fprintf (stderr, \"%s = %s\\n\", cols[i], argv[i] ? argv[i] : \"NULL\");
    }
    fprintf (stderr, \"\\n\");
  }
  struct t_arr_""" + table_name["c_name"] + """ *a = _r;
  if (a->cur_num == a->max_num) { return 0;}
  struct """ + table_name["c_name"] + """ *r = malloc (sizeof (*r));
  a->r[a->cur_num ++] = r;
""" + s + """
  return 0;
}
""")

  s = "SELECT * FROM " + table_name["sql_name"] + " WHERE id = %d LIMIT 1;"

  h_file.write ("struct " + table_name["c_name"] + " *vk_db_lookup_" + table_name["c_name"] + " (int id);\n")
  c_file.write ("""
struct """ + table_name["c_name"] + """ *vk_db_lookup_""" + table_name["c_name"] + """ (int id) {
  struct t_arr_""" + table_name["c_name"] + """ r;
  r.cur_num = 0;
  r.max_num = 1;
  r.r = malloc (sizeof (void *) * r.max_num);

  char *q = sqlite3_mprintf (\"""" + s + """\", id);
  assert (q);
  if (verbosity >= 4) {
    fprintf (stderr, \"%s\\n\", q);
  }
  char *e = 0;

  if (sqlite3_exec (db_handle, q, ct_""" + table_name["c_name"] + """_callback, &r, &e) != SQLITE_OK || e) {
    vk_error (ERROR_SQLITE, \"SQLite error %s\\n\", e);
    free (r.r);
    return 0;
  }

  sqlite3_free (q);

  if (r.cur_num == 0) {
    free (r.r);
    return 0;
  } else {
    struct """ + table_name["c_name"] + """ *t = r.r[0];
    free (r.r);
    return t;
  }
} 
""")

  s = ""
  ok = 0
  for f in fields:
    if (f["c_type"] == "int"):
      s += "  x = a->" + f["c_name"] + " - b->" + f["c_name"] + ";\n"
    elif (f["c_type"] == "char *"):
      s += "  x = strcmp (a->" + f["c_name"] + ", b->" + f["c_name"] + ");\n"
    s += "  if (x) { return x;}\n"

  h_file.write ("int cmp_" + table_name["c_name"] + " (struct " + table_name["c_name"] + " *a, struct " + table_name["c_name"] + " *b);\n")
  c_file.write ("""
int cmp_""" + table_name["c_name"] + """ (struct """ + table_name["c_name"] + """ *a, struct """ + table_name["c_name"] + """ *b) {
  int x;
""" + s + """
  return 0;
}
""")


def nt (table_name):
  r = {}
  r["c_name"] = table_name[0].strip ()
  r["sql_name"] = table_name[1].strip ()
  r["flags"] = table_name[2].strip ()
  assert (len (table_name) == 3)
  return r
  
def gen_sql_type (c_type):
  if (c_type == "int"):
    return "INT"
  elif (c_type == "char *"):
    return "VARCHAR"
  else:
    assert (0)

def gen_json_type (c_type):
  if (c_type == "int"):
    return "integer"
  elif (c_type == "char *"):
    return "string"
  else:
    assert (0)

def nf (f):
  r = {}
  r["c_name"] = f[0].strip ()
  r["c_type"] = f[1].strip ()
  r["sql_name"] = f[2].strip ()
  if (r["sql_name"] == ""):
    r["sql_name"] = r["c_name"]
  r["sql_type"] = f[3].strip ()
  if (r["sql_type"] == ""):
    r["sql_type"] = gen_sql_type (r["c_type"])
  r["json_name"] = f[4].strip ()
  if (r["json_name"] == ""):
    r["json_name"] = r["c_name"]
  r["json_type"] = f[5].strip ()
  if (r["json_type"] == ""):
    r["json_type"] = gen_json_type (r["c_type"])
  r["flags"] = int (f[6].strip ())
  assert (len (f) == 7)
  return r

assert (len (sys.argv) >= 3)

h_file = open (sys.argv[1], "w")
c_file = open (sys.argv[2], "w")

h_file.write ("""
#ifndef __AUTOGENERATED_H__
#define __AUTOGENERATED_H__

#include <jansson.h>
""")

c_file.write ("""
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
      fprintf (stderr, "%s = %s\\n", cols[i], argv[i] ? argv[i] : "NULL");
    }
    fprintf (stderr, "\\n");
  }
  return 0;
}
""")

for fl in sys.argv[3:]:
  f = open (fl, "r")


  table_name = f.readline ().strip ().split (":")
  table_name = nt (table_name)

  fields = []
  for line in f:
    l = line.strip ()
    if l.startswith ("//"):
      continue
    o = l.split (":")
    fields.append (nf (o))

  create_table (table_name, fields)
  f.close ()
#pprint.pprint (fields)

h_file.write ("#endif\n")
