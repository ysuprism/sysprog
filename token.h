
typedef enum {
  TKN_NONE,
  TKN_NORMAL,
  TKN_EOF,
  TKN_EOL,
  TKN_BG,
  TKN_PIPE,
  TKN_REDIR_IN,
  TKN_REDIR_OUT,
  TKN_REDIR_APPEND
} token;