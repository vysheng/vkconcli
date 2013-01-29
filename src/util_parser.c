#include <string.h>
#include "util_tokenizer.h"
#include "net.h"
#include "vk_errors.h"
#include "util_config.h"
#include "tree.h"

#define NT \
  { \
    int t = next_token ();\
    if (t != TOKEN_OK) { return t;}\
  }

int var_limit;
int limit;

void clear_vars (void) {
  var_limit = 0;
}

int work_console_msg_to (int id) {
  NT;
  char *s = strndup (cur_token, cur_token_len);
  //vk_msg_send (id, s);
  if (aio_msg_send (id, s) < 0) {
    printf ("Not sent.\n");
    clear_vars ();
    return _ERROR;
  }
  //printf ("Msg to %d successfully sent:\n---\n%s\n---\n", id, s);
  free (s);
  clear_vars ();
  return TOKEN_OK;
}

int work_console_msg (void) {
  NT;
  int n = atoi (cur_token);
  if (n <= 0 || n >= 1000000000) { return TOKEN_UNEXPECTED; }
  return work_console_msg_to (n);
}

int work_console_history (void) {
  limit = var_limit ? var_limit : default_history_limit;
  return TOKEN_OK;
}

int work_console_alias (void) {
  NT;
  static char *name;
  if (name) { free (name); }
  name = strndup (cur_token, cur_token_len);
  NT;
  char *text = strndup (cur_token, cur_token_len);
  if (vk_alias_add (name, text) < 0) {
    free (text);
    clear_vars ();
    return _ERROR;
  } else {
    free (text);
    clear_vars ();
    return TOKEN_OK;
  }
}

int work_console_force_update (void) {
  if (aio_force_update () < 0) {
    clear_vars ();
    return _ERROR;
  } else {
    clear_vars ();
    return TOKEN_OK;
  }
}

int work_set_limit (int s) {
  var_limit = atoi (cur_token + s);
  if (var_limit <= 0 || var_limit >= 1000) {
    var_limit = 0;
  }
  return 0;
}

int work_parser (void) {
  NT;
  if (cur_token_len == 3 && !strncmp ("msg", cur_token, 3)) {
    return work_console_msg ();
  } else if (cur_token_len == 7 && !strncmp ("history", cur_token, 7)) {
    return work_console_history ();
  } else if (cur_token_len == 5 && !strncmp ("alias", cur_token, 5)) {
    return work_console_alias ();
  } else if (cur_token_len == 12 && !strncmp ("force_update", cur_token, 12)) {
    return work_console_force_update ();
  } else if (cur_token_len >= 2 && !strncmp ("l=", cur_token, 2)) {
    return work_set_limit (2);
  } else if (cur_token_len >= 6 && !strncmp ("limit=", cur_token, 6)) {
    return work_set_limit (6);
  }
  return TOKEN_UNEXPECTED;
}
