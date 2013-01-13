#define STD_BUF_SIZE (1 << 20)
#define _wint_t wint_t
char std_buf[STD_BUF_SIZE + 1];
int std_buf_pos;

_wint_t token_buf[MAX_TOKEN_SIZE + 4];
int token_buf_pos;
enum token_parse_state {
  parse_any,
  parse_11_string,
  parse_13_string,
  parse_21_string,
  parse_23_string,
  parse_regular
};

enum token_parse_state token_parse_state;

enum multichar_state {
  ms_none,
  ms_backslash,
  ms_td1,
  ms_td2,
  ms_ts1,
  ms_ts2
};
enum multichar_state multichar_state;


char unicode_buf[8];
int unicode_pos;
int unicode_state;

void process_global_char (_wint_t c) {
  switch (token_parse_state) {
  case parse_any:
    assert (!token_buf_pos);
    switch (c) {
    case MC_TRIPLE_DOUBLE_QUOTE:
      token_parse_state = parse_23_string;
      return;
    case MC_TRIPLE_SINGLE_QUOTE:
      token_parse_state = parse_13_string;
      return;
    case ''':
      token_parse_state = parse_11_string;
      return;
    case '"':
      token_parse_state = parse_21_string;
      return;
    case ' ':
    case '\t':
      return;
    case '\n':
    case '\r':
      add_soft_end_token ();
      return;
    case ';':
      add_hard_end_token ();
      return;
    default:
      token_parse_state = parse_regular;        
      if (token_buf_pos < MAX_TOKEN_SIZE) {
        token_buf[token_buf_pos ++] = c;
      }
      return;
    }
  case parse_11_string:
    switch (c) {
    case ''':
      add_token ();
      token_parse_state = parse_any;
      return;
    case '\n':
    case '\r':
      add_error_token ();
      vk_error (ERROR_INPUT, "Line break in single-quoted string");
      return;
    default:
      if (token_buf_pos < MAX_TOKEN_SIZE) {
        token_buf[token_buf_pos ++] = c;
      }
      return;      
    }
  case parse_21_string:
    switch (c) {
    case '"':
      add_token ();
      token_parse_state = parse_any;
      return;
    case '\n':
    case '\r':
      add_error_token ();
      vk_error (ERROR_INPUT, "Line break in single-quoted string");
      return;
    default:
      if (token_buf_pos < MAX_TOKEN_SIZE) {
        token_buf[token_buf_pos ++] = c;
      }
      return;      
    }
  case parse_13_string:
    switch (c) {
    case MC_TRIPLE_SINGLE_QUOTE:
      add_token ();
      token_parse_state = parse_any;
      return;
    default:
      if (token_buf_pos < MAX_TOKEN_SIZE) {
        token_buf[token_buf_pos ++] = c;
      }
      return;      
    }
  case parse_23_string:
    switch (c) {
    case MC_TRIPLE_DOUBLE_QUOTE:
      add_token ();
      token_parse_state = parse_any;
      return;
    default:
      if (token_buf_pos < MAX_TOKEN_SIZE) {
        token_buf[token_buf_pos ++] = c;
      }
      return;      
    }
  case parse_regular:
    switch (c) {
    case ' ':
    case '\t':
      add_token ();
      token_parse_state = parse_any;
      return;
    case '\n':
    case '\r':
      add_token ();
      add_soft_end_token ();
      token_parse_state = parse_any;
      return;
    case ';':
      add_token ();
      add_hard_end_token ();
      token_parse_state = parse_any;
      return;
    default:
      if (token_buf_pos < MAX_TOKEN_SIZE) {
        token_buf[token_buf_pos ++] = c;
      }
      return;
    }
  }
}

void process_any_char (_wint_t c) {
  int ab = (token_parse_state != parse_21_string) && (token_parse_state != parse_23_string);
  int atd = token_parse_state == parse_any || token_parse_state == parse_23_string;
  int ats = token_parse_state == parse_any || token_parse_state == parse_13_string;
  switch (multichar_state) {
  case ms_backslash:
    multichar_state = ms_none;
    switch (c) {
    case 'n':
      process_global_char ('\n' | MC_ESCAPED | MC_CHANGED);
      break;
    case 'r':
      process_global_char ('\r' | MC_ESCAPED | MC_CHANGED);
      break;
    case 't':
      process_global_char ('\t' | MC_ESCAPED | MC_CHANGED);
      break;
    default:
      process_global_char (c | MC_ESCAPED);
    }
    return;
  case ms_td1:
    if (c == '"') {
      multichar_state ++;
      return;
    } else {
      process_global_char ('"');
      process_global_char (c);
      multichar_state = ms_none;
      return;
    }
  case ms_td2:
    if (c == '"') {
      process_global_char (MC_TRIPLE_DOUBLE_QUOTE);
      multichar_state = ms_none;
      return;
    } else {
      process_global_char ('"');
      process_global_char ('"');
      process_global_char (c);
      multichar_state = ms_none;
      return;
    }
  case ms_ts1:
    if (c == ''') {
      multichar_state ++;
      return;
    } else {
      process_global_char (''');
      process_global_char (c);
      multichar_state = ms_none;
      return;
    }
  case ms_td2:
    if (c == ''') {
      process_global_char (MC_TRIPLE_SINGLE_QUOTE);
      multichar_state = ms_none;
      return;
    } else {
      process_global_char (''');
      process_global_char (''');
      process_global_char (c);
      multichar_state = ms_none;
      return;
    }
  case ms_none:
    if (ab && c == '\\') {
      multichar_state = ms_backslash;
    } else if (atd && c == '"') {
      multichar_state = ms_td1;
    } else if (ats && c == ''') {
      multichar_state = ms_ts1;
    } else {
      process_global_char (c);
    }
  }
}

void process_unicode_char (unsigned char c) {
  if (!unicode_pos) {
    unicode_state = get_unicode_len (c);
    if (unicode_state <= 0) {
      vk_error (VK_ERROR_INPUT, "Invalid unicode symbol: first byte: %08b", (unsigned)c);
      return;
    }
    if (unicode_state >= 4) {
      vk_error (VK_ERROR_INPUT, "Supported only symbols with length <= 3: first byte: %08b", (unsigned)c);
      return;
    }
    assert (unicode_state <= 3);
    unicode_state --;
    unicode_buf[unicode_pos ++] = c;
  } else {
    if ((c & 0xc0) != 0xc0) {
      vk_error (VK_ERROR_INPUT, "Invalid unicode symbol: byte #%d: %08b", unicode_pos + 1, (unsigned)c);
    }
    unicode_state --;
    unicode_buf[unicode_pos ++] = c;
  }
  if (!unicode_state) {
    int i;
    _wint_t a;
    for (i = unicode_pos - 1; i >= 0; i--) {
      a = (a << 8) + (unsigned char)unicode_buf[i];
    }
    unicode_pos = 0;
    unicode_state = 0;
    process_any_char (a);
  }
}

void process_char (unsigned char c) {
  if (unicode_mode) {
    process_unicode_char (c);
  } else {
    process_any_char (c);
  }
}

void work_console (void) {
  int x = read (STDIN_FILENO, std_buf + std_buf_pos, STD_BUF_SIZE - std_buf_pos);
  if (x <= 0) {
    return;
  }
  for (i = 0; i < x; i++) {
    process_char (std_buf[i]);
  }
}
