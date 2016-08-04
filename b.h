/* Copyright 2016 Connor Taffe */

#ifndef B_B_H_
#define B_B_H_

#include <stdbool.h>
#include <stddef.h>

struct b_token {
  enum {
    B_TOK_NONE,
    B_TOK_ERR,
    B_TOK_WHITESPACE,
    B_TOK_LPAREN,
    B_TOK_RPAREN,
    B_TOK_COMMA,
    B_TOK_IDENT,
    B_TOK_OPER,
    B_TOK_max
  } type;
  char *buf;
  size_t sz;
};

struct b_token_list {
  struct b_token_list *next;
  struct b_token tok;
};

struct b_lex {
  char *message;
  int sock;
  size_t sz, cap, l, i;
  struct b_token_list *list;
};

typedef int(birch_message_handlefunc)(void *o, struct b_token_list *list);

struct birch_message_handler {
  void *obj;
  birch_message_handlefunc *func;
};

/* NOTE: must be restartable */
typedef int(b_lex_statefunc)(struct b_lex *l);
typedef bool(b_lex_checkfunc)(char c);

/* NDFA graph */
struct b_ndfa {
  b_lex_statefunc *func;
  struct b_ndfa **next;
  size_t sz;
};

int b_state_machine(struct b_lex *l, struct b_ndfa *ndfa);

int b_lex_next(struct b_lex *l, char *c);
void b_lex_back(struct b_lex *l);
void b_lex_emit(struct b_lex *l, struct b_token tok);
void b_lex_buf(struct b_lex *l, char **buf, size_t *sz);

b_lex_statefunc b_lex_state_start;
b_lex_statefunc b_lex_state_ident;
b_lex_statefunc b_lex_state_whitespace;
b_lex_statefunc b_lex_state_function_call;
b_lex_statefunc b_lex_state_function_call_args;
b_lex_statefunc b_lex_state_function_call_args_post;

b_lex_checkfunc b_character_is_letter;
b_lex_checkfunc b_character_is_whitespace;

#endif /* B_B_H_ */
