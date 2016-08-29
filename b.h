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
    B_TOK_NUMBER,
    B_TOK_STR,
    B_TOK_OPER,
    B_TOK_max
  } type;
  char *buf;
  size_t sz, i, line, col;
};

struct b_token_list {
  struct b_token_list *next;
  struct b_token tok;
};

struct b_lex {
  char *message;
  int sock;
  size_t sz, cap, l, i, line, col, lline, lcol;
  struct b_token_list *head, *list;
  int pdepth;
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
void b_lex_tok(struct b_lex *l, struct b_token *tok);
size_t b_lex_len(struct b_lex *l);

enum {
  B_LEX_STATE_EXPR_ERR,
  B_LEX_STATE_EXPR_LETTER,
  B_LEX_STATE_EXPR_DIGIT,
  B_LEX_STATE_EXPR_WHITESPACE,
  B_LEX_STATE_EXPR_LPAREN,
  B_LEX_STATE_EXPR_RPAREN,
  B_LEX_STATE_EXPR_QUOTE,
  B_LEX_STATE_EXPR_max
};

enum {
  B_LEX_STATE_NUMBER_ERR,
  B_LEX_STATE_NUMBER_NUMBER,
  B_LEX_STATE_NUMBER_max
};

enum {
  B_LEX_STATE_STR_ERR,
  B_LEX_STATE_STR_NOSTR,
  B_LEX_STATE_STR_STR,
  B_LEX_STATE_STR_max
};

enum {
  B_LEX_STATE_TUPLE_NOTUPLE,
  B_LEX_STATE_TUPLE_TUPLE,
  B_LEX_STATE_TUPLE_max
};

enum {
  B_LEX_STATE_OPERATOR_ERR,
  B_LEX_STATE_OPERATOR_NOOPERATOR,
  B_LEX_STATE_OPERATOR_WHITESPACE,
  B_LEX_STATE_OPERATOR_DOT,
  B_LEX_STATE_OPERATOR_COMMA,
  B_LEX_STATE_OPERATOR_LPAREN,
  B_LEX_STATE_OPERATOR_RPAREN,
  B_LEX_STATE_OPERATOR_max
};

enum { B_LEX_STATE_WHITESPACE_WHITESPACE, B_LEX_STATE_WHITESPACE_max };

enum { B_LEX_STATE_IDENT_ERR, B_LEX_STATE_IDENT_IDENT, B_LEX_STATE_IDENT_max };

b_lex_statefunc b_lex_state_expr;
b_lex_statefunc b_lex_state_ident;
b_lex_statefunc b_lex_state_number;
b_lex_statefunc b_lex_state_str;
b_lex_statefunc b_lex_state_tuple;
b_lex_statefunc b_lex_state_whitespace;
b_lex_statefunc b_lex_state_function_call;
b_lex_statefunc b_lex_state_operator;

b_lex_checkfunc b_character_is_letter;
b_lex_checkfunc b_character_is_digit;
b_lex_checkfunc b_character_is_whitespace;

extern struct b_ndfa b_lex_states[];

int b_new_lexer(struct b_lex **l);

#endif /* B_B_H_ */
