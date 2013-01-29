#include "util_tokenizer.h"
#include "util_parser.h"
#include "tree.h"
#include <string.h>
#include <assert.h>
#include <vk_errors.h>


#define MAX_DEPTH 10

int work_console_main (void);




int parse_all_tokens (const char *name, int depth, int flags);

int cur_pos;
char *cur_token;
int cur_token_len;
int cur_token_flags;

int is_whitespace (char c) {
  return (c == 32 || c == 10 || c == 13 || c == 9 || c == 11 || c == 12);
}

int is_linebreak (char c) {
  return (c == 10 || c == 13);
}

struct token *tokens;
struct token *last_token;
struct token *ctoken;

int parse_all_tokens (const char *name, int depth, int flags);
// name should be duplicated
void add_token (char *name, int flags, int depth) {
  if (!(flags & 1) && depth == MAX_DEPTH) {
    flags |= 3;
  }
  if (!(flags & 1)) {
    char *res = vk_alias_get (name);
    if (res) {
      free (name);
      parse_all_tokens (res, depth, 0);
      free (res);
      return;
    }
  }
  struct token *token = malloc (sizeof (*token));
  token->str = name;
  token->len = strlen (name);
  token->flags = flags;
  token->next = 0;
  if (last_token) {
    last_token->next = token;
    last_token = token;
  } else {
    assert (!tokens);
    ctoken = tokens = last_token = token;
  }
}

int parse_all_tokens (const char *name, int depth, int flags) {
  #define c name[pos]
  int pos = 0;
  while (c) {
    while (is_whitespace (c)) { pos ++; }
    if (!c) { return pos; }
    const char *p = name + pos;
    if (c != '"') {
      while (!is_whitespace (c) && c) { pos ++; }
      add_token (strndup (p, pos + name - p), 0, depth + 1);
      if (!c) {
        return pos;
      }
    } else {
      if (name[pos + 1] == '"' && name[pos + 2] == '"') {
        pos += 3;
        p += 3;
        while (c && memcmp ("\"\"\"", name + pos, 3)) { pos ++; }
        if (!c && !(flags & 1)) {
          vk_error (1, "incorrect token `%s`\n", p - 3); 
        } else {
          if (c) {
            add_token (strndup (p, pos + name - p), 1, depth + 1);
          } else {
            return p - name - 3;
          }
        }
      } else {
        p ++;
        pos ++;
        while (!is_linebreak (c) && c != '"' && c) { pos ++; }
        if (c == '"') {
          add_token (strndup (p, pos + name - p), 1, depth + 1);
        } else {
          if (c || (!c && !(flags & 1))) {
            vk_error (1, "incorrent token `%.*s`\n", (int)(pos + name - p + 1), p);
          } else {
            return p - name - 1;
          }
        }
      }
    }
  }
  return pos;
  #undef c
}

void free_unused_tokens (void) {
  while (tokens != ctoken) {
    free (tokens->str);
    struct token *t = tokens;
    tokens = tokens->next;
    free (t);
  }
  if (!tokens) {
    last_token = 0;
  }
}

int next_token (void) {
  if (!ctoken) {
    return TOKEN_NEED_MORE_BYTES;
  }
  cur_token = ctoken->str;
  cur_token_len = ctoken->len;
  cur_token_flags = ctoken->flags;
  vk_log (2, "next_token = %s\n", cur_token);
  ctoken = ctoken->next;
  return TOKEN_OK;
}



int work_tokenizer (const char *buf) {
  int r = parse_all_tokens (buf, -1, 1);
  //work_parser ();
  while (1) {
    //int l = work_console_main ();
    int l = work_parser ();
    if (l != TOKEN_NEED_MORE_BYTES) {
      free_unused_tokens ();
    } else {
      ctoken = tokens;
      break;
    }
  }
  return r;
}
