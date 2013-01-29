#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <assert.h>
#include <memory.h>
#include <malloc.h>

#ifdef ENABLE_READLINE
#  include <readline/readline.h>
#  include <readline/history.h>
#  include <unistd.h>
#  include <termios.h>
#endif

#include "util_tokenizer.h"

void output_string (const char *s) {
  assert (s);
#ifdef ENABLE_READLINE
  rl_save_prompt ();
  rl_clear_message ();
  rl_message ("\033[K%s\n", s);
  //rl_crlf ();
  rl_restore_prompt ();
  rl_redisplay ();  
#else
  printf ("%s\n", s);
#endif
}

#ifdef ENABLE_READLINE
int readline_new_data;
#endif

#ifdef ENABLE_READLINE
struct termios save_tty, cur_tty;

#define STD_BUF_SIZE (1 << 20)
char std_buf[STD_BUF_SIZE + 2];
int std_buf_pos;

void readline_callback (char *s) {
  if (s) {
    if (*s) {
      add_history (s);
    }
    int l = strlen (s);
    assert (l + std_buf_pos + 2 < STD_BUF_SIZE);
    memcpy (std_buf + std_buf_pos, s, l);
    std_buf_pos += l;
    std_buf[std_buf_pos ++] = '\n';
    std_buf[std_buf_pos] = 0;
    readline_new_data ++;
    free (s);
  }
}

void init_io (void) {
  rl_callback_handler_install ("Enter command>  ", readline_callback);
  tcgetattr (STDIN_FILENO, &save_tty);
  cur_tty = save_tty;
  cur_tty.c_lflag &= ~ECHO;
  cur_tty.c_lflag &= ~ICANON;
  tcsetattr (STDIN_FILENO, 0, &cur_tty);
//  rl_restore_prompt ();  
}

void restore_io (void) {
  tcsetattr (STDIN_FILENO, 0, &save_tty);
}
#else
void init_io (void) {
}

void restore_io (void) {
}
#endif


void work_io (void) {
#ifndef ENABLE_READLINE
  int x = read (STDIN_FILENO, std_buf + std_buf_pos, STD_BUF_SIZE - std_buf_pos);
  if (x <= 0) {
    return;
  }
  std_buf_pos += x;
  std_buf[std_buf_pos] = 0;
#else
  //rl_save_prompt ();
  //rl_restore_prompt ();  
  rl_callback_read_char ();  
  if (!readline_new_data) { return; }
#endif
  //int l = parse_all_tokens (std_buf, -1, 1);
  int l = work_tokenizer (std_buf);
  memcpy (std_buf, std_buf + l, std_buf_pos - l + 1);
  std_buf_pos -= l;

#ifdef ENABLE_READLINE
  readline_new_data = 0;
#endif
}
