#ifndef __UTIL_TOKENIZER_H__
#define __UTIL_TOKENIZER_H__

#define TOKEN_NEED_MORE_BYTES -1
#define TOKEN_ERROR -2
#define TOKEN_OK 1 
#define TOKEN_UNEXPECTED -3
struct token {
  char *str;
  int len;
  int flags;
  struct token *next;
};

//extern struct token *cur_token;
extern char *cur_token;
extern int cur_token_len;


int next_token (void);
void add_token (char *name, int flags, int depth);

int work_tokenizer (const char *buf);
#endif
