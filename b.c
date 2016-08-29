/* Copyright 2016 Connor Taffe */

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "b.h"

/* places the next byte in c, returns -1 if no more characters
   are lexable */
int b_lex_next(struct b_lex *l, char *c) {
  ssize_t ret;

  assert(l != 0);

  /* increase buffer size as needed */
  if (l->sz == l->cap) {
    l->cap = 2 * l->cap + 1;
    l->message = realloc(l->message, l->cap);
    assert(l->message != 0);
  }

  if (l->i == l->sz) {
    if (l->sock == -1)
      return -1;
    ret = read(l->sock, &l->message[l->sz], l->cap - l->sz);
    assert(ret != -1);
    if (ret == 0)
      return -1;
    l->sz += (size_t)ret;
  }

  if (l->i < l->sz) {
    if (c != 0)
      *c = l->message[l->i];
    l->i++;
    if (*c == '\n') {
      l->line++;
      l->col = 0;
    } else
      l->col++;
    return 0;
  }
  return -1;
}

void b_lex_back(struct b_lex *l) {
  assert(l != 0);

  if (l->i > l->l)
    l->i--;
  if (!l->col)
    l->line--;
  else
    l->col--;
}

void b_lex_emit(struct b_lex *l, struct b_token tok) {
  struct b_token_list *link;

  assert(l != 0);

  link = calloc(sizeof(struct b_token_list), 1);
  assert(link != 0);
  link->tok = tok;
  if (l->list)
    l->list->next = link;
  l->list = link;
  if (!l->head)
    l->head = link;
}

/* gets & resets buffer */
void b_lex_tok(struct b_lex *l, struct b_token *t) {
  assert(l != 0);

  if (l->i == l->l)
    return;

  t->sz = l->i - l->l;
  t->buf = calloc(sizeof(char), t->sz);
  t->line = l->lline;
  t->col = l->lcol;
  assert(t->buf != 0);
  memcpy(t->buf, &l->message[l->l], t->sz);
  l->l = l->i; /* reset buffer */
  l->lline = l->line;
  l->lcol = l->col;
}

size_t b_lex_len(struct b_lex *l) { return l->i - l->l; }

int b_state_machine(struct b_lex *l, struct b_ndfa *g) {
  int next;

  for (; g && (size_t)(next = g->func(l)) < g->sz; g = g->next[next])
    ;

  return -1;
}

bool b_character_is_letter(char c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

bool b_character_is_digit(char c) { return c >= '0' && c <= '9'; }

bool b_character_is_whitespace(char c) {
  return c == ' ' || c == '\t' || c == '\n';
}

int b_lex_state_ident(struct b_lex *l) {
  char c;

  assert(l);

  for (;;) {
    if (b_lex_next(l, &c) == -1)
      return -1;
    if (b_character_is_letter(c))
      ;
    else {
      /* emit ident */
      struct b_token tok;
      memset(&tok, 0, sizeof(tok));

      b_lex_back(l);

      if (!b_lex_len(l)) {
        b_lex_tok(l, &tok);
        free(tok.buf);
        tok.type = B_TOK_ERR;
        tok.buf = "Expected identifier";
        tok.sz = strlen(tok.buf);
        b_lex_emit(l, tok);
        return B_LEX_STATE_IDENT_ERR;
      }

      b_lex_tok(l, &tok);
      tok.type = B_TOK_IDENT;
      b_lex_emit(l, tok);

      return B_LEX_STATE_IDENT_IDENT;
    }
  }
}

/* operator */
int b_lex_state_operator(struct b_lex *l) {
  char c;
  struct b_token tok;

  assert(l);

  memset(&tok, 0, sizeof(tok));

  for (;;) {
    if (b_lex_next(l, &c) == -1)
      return -1;
    if (b_character_is_whitespace(c))
      return B_LEX_STATE_OPERATOR_WHITESPACE;
    else if (c == '.') {
      /* emit dot operator */

      b_lex_tok(l, &tok);
      tok.type = B_TOK_OPER;
      b_lex_emit(l, tok);

      return B_LEX_STATE_OPERATOR_DOT;
    } else if (c == ',') {

      b_lex_tok(l, &tok);
      tok.type = B_TOK_COMMA;
      b_lex_emit(l, tok);

      return B_LEX_STATE_OPERATOR_COMMA;
    } else if (c == '(') {
      /* emit opening paren */

      l->pdepth++;

      b_lex_tok(l, &tok);
      tok.type = B_TOK_LPAREN;
      b_lex_emit(l, tok);

      return B_LEX_STATE_OPERATOR_LPAREN;
    } else if (c == ')') {
      /* emit right parenthesis */

      if (l->pdepth > 0) {
        l->pdepth--;

        b_lex_tok(l, &tok);
        tok.type = B_TOK_RPAREN;
        b_lex_emit(l, tok);

        return B_LEX_STATE_OPERATOR_RPAREN;
      } else {
        /* error */
        char *buf = "Too many closing parens.";

        b_lex_tok(l, &tok);
        free(tok.buf);
        tok.type = B_TOK_ERR;
        tok.buf = buf;
        tok.sz = strlen(buf);
        b_lex_emit(l, tok);
        return B_LEX_STATE_OPERATOR_ERR;
      }
    } else
      /* operators don't *have* to be found */
      return b_lex_back(l), B_LEX_STATE_OPERATOR_NOOPERATOR;
  }
}

int b_lex_state_tuple(struct b_lex *l) {
  char c;
  struct b_token tok;

  assert(l);

  memset(&tok, 0, sizeof(tok));

  if (b_lex_next(l, &c) == -1)
    return -1;
  if (c == '(') {
    /* emit opening paren */

    l->pdepth++;

    b_lex_tok(l, &tok);
    tok.type = B_TOK_LPAREN;
    b_lex_emit(l, tok);

    return B_LEX_STATE_TUPLE_TUPLE;
  } else
    return b_lex_back(l), B_LEX_STATE_TUPLE_NOTUPLE;
}

int b_lex_state_str(struct b_lex *l) {
  char c;
  struct b_token tok;

  assert(l);

  memset(&tok, 0, sizeof(tok));

  if (b_lex_next(l, &c) == -1)
    return -1;
  if (c == '"') {
    /* parse string */
    for (;;) {
      if (b_lex_next(l, &c) == -1)
        return -1;
      if (c == '"')
        break;
      else if (c == '\n') {
        /* newlines not allowed in strings */
        b_lex_tok(l, &tok);
        free(tok.buf);
        tok.type = B_TOK_ERR;
        tok.buf = "Newlines not allowd in strings, try using a raw string.";
        tok.sz = strlen(tok.buf);
        b_lex_emit(l, tok);

        return B_LEX_STATE_STR_ERR;
      }
    }
  } else if (c == '`') {
    /* parse raw string */
    for (;;) {
      if (b_lex_next(l, &c) == -1)
        return -1;
      if (c == '`')
        break;
    }
  } else if (c == '\'') {
    /* parse single character */
    if (b_lex_next(l, &c) == -1)
      return -1;
    if (b_lex_next(l, &c) == -1)
      return -1;
    if (c != '\'') {
      /* error */
      char *buf, *fmt;
      int sz;

      fmt = "Unexpected character '%c', expected closing character quote.";
      sz = snprintf(0, 0, fmt, c);
      if (sz < 0)
        return -1;
      buf = calloc(sizeof(char), (size_t)sz);
      snprintf(buf, (size_t)sz, fmt, c);

      b_lex_tok(l, &tok);
      free(tok.buf);
      tok.type = B_TOK_ERR;
      tok.buf = buf;
      tok.sz = (size_t)sz;
      b_lex_emit(l, tok);

      return B_LEX_STATE_STR_ERR;
    }
  } else
    return B_LEX_STATE_STR_NOSTR; /* not a string */

  b_lex_tok(l, &tok);
  tok.type = B_TOK_STR;
  b_lex_emit(l, tok);

  return B_LEX_STATE_STR_STR;
}

int b_lex_state_number(struct b_lex *l) {
  char c;

  assert(l);

  for (;;) {
    if (b_lex_next(l, &c) == -1)
      return -1;
    if (b_character_is_digit(c))
      ;
    else {
      /* emit number */
      struct b_token tok;

      memset(&tok, 0, sizeof(tok));

      b_lex_back(l);

      if (!b_lex_len(l)) {
        b_lex_tok(l, &tok);
        free(tok.buf);
        tok.type = B_TOK_ERR;
        tok.buf = "Expected identifier";
        tok.sz = strlen(tok.buf);
        b_lex_emit(l, tok);
        return B_LEX_STATE_NUMBER_ERR;
      }

      b_lex_tok(l, &tok);
      tok.type = B_TOK_NUMBER;
      b_lex_emit(l, tok);

      return B_LEX_STATE_NUMBER_NUMBER;
    }
  }
}

int b_lex_state_whitespace(struct b_lex *l) {
  char c;

  assert(l);

  for (;;) {
    if (b_lex_next(l, &c) == -1)
      return -1;
    else if (b_character_is_whitespace(c))
      ;
    else {
      struct b_token tok;

      memset(&tok, 0, sizeof(tok));

      b_lex_back(l);

      b_lex_tok(l, &tok);
      tok.type = B_TOK_WHITESPACE;
      b_lex_emit(l, tok);

      return B_LEX_STATE_WHITESPACE_WHITESPACE;
    }
  }
}

int b_lex_state_expr(struct b_lex *l) {
  char c;

  assert(l);

  for (;;) {
    if (b_lex_next(l, &c) == -1)
      return -1;
    if (b_character_is_letter(c))
      return b_lex_back(l), B_LEX_STATE_EXPR_LETTER;
    else if (b_character_is_digit(c))
      return b_lex_back(l), B_LEX_STATE_EXPR_DIGIT;
    else if (b_character_is_whitespace(c))
      return b_lex_back(l), B_LEX_STATE_EXPR_WHITESPACE;
    else if (c == '(')
      return b_lex_back(l), B_LEX_STATE_EXPR_LPAREN;
    else if (c == ')')
      return b_lex_back(l), B_LEX_STATE_EXPR_RPAREN;
    else if (c == '"' || c == '`' || c == '\'')
      return b_lex_back(l), B_LEX_STATE_EXPR_QUOTE;
    else {
      /* error */
      char *buf, *fmt;
      struct b_token tok;
      int sz;

      memset(&tok, 0, sizeof(tok));

      fmt = "Unexpected character '%c', expected identifier.";
      sz = snprintf(0, 0, fmt, c);
      if (sz < 0)
        return -1;
      buf = calloc(sizeof(char), (size_t)sz);
      snprintf(buf, (size_t)sz, fmt, c);

      b_lex_tok(l, &tok);
      free(tok.buf);
      tok.type = B_TOK_ERR;
      tok.buf = buf;
      tok.sz = (size_t)sz;
      b_lex_emit(l, tok);

      return B_LEX_STATE_EXPR_ERR;
    }
  }
}

/* lexical states */
enum {
  B_LEX_STATES_STATE_EXPR,
  B_LEX_STATES_STATE_IDENT,
  B_LEX_STATES_STATE_WHITESPACE1,
  B_LEX_STATES_STATE_WHITESPACE2,
  B_LEX_STATES_STATE_OPERATOR,
  B_LEX_STATES_STATE_NUMBER,
  B_LEX_STATES_STATE_TUPLE,
  B_LEX_STATES_STATE_STR,
  B_LEX_STATES_STATE_max
};

struct b_ndfa b_lex_states[B_LEX_STATES_STATE_max];

__attribute__((constructor)) static void b_lex_states_init(void);

/* Initialize lexer mapping */
void b_lex_states_init() {
  size_t i;

  memset(b_lex_states, 0, sizeof(b_lex_states));

  b_lex_states[B_LEX_STATES_STATE_EXPR].func = b_lex_state_expr;
  b_lex_states[B_LEX_STATES_STATE_EXPR].sz = B_LEX_STATE_EXPR_max;

  b_lex_states[B_LEX_STATES_STATE_IDENT].func = b_lex_state_ident;
  b_lex_states[B_LEX_STATES_STATE_IDENT].sz = B_LEX_STATE_IDENT_max;

  b_lex_states[B_LEX_STATES_STATE_WHITESPACE1].func = b_lex_state_whitespace;
  b_lex_states[B_LEX_STATES_STATE_WHITESPACE1].sz = B_LEX_STATE_WHITESPACE_max;

  b_lex_states[B_LEX_STATES_STATE_WHITESPACE2].func = b_lex_state_whitespace;
  b_lex_states[B_LEX_STATES_STATE_WHITESPACE2].sz = B_LEX_STATE_WHITESPACE_max;

  b_lex_states[B_LEX_STATES_STATE_OPERATOR].func = b_lex_state_operator;
  b_lex_states[B_LEX_STATES_STATE_OPERATOR].sz = B_LEX_STATE_OPERATOR_max;

  b_lex_states[B_LEX_STATES_STATE_NUMBER].func = b_lex_state_number;
  b_lex_states[B_LEX_STATES_STATE_NUMBER].sz = B_LEX_STATE_NUMBER_max;

  b_lex_states[B_LEX_STATES_STATE_TUPLE].func = b_lex_state_tuple;
  b_lex_states[B_LEX_STATES_STATE_TUPLE].sz = B_LEX_STATE_TUPLE_max;

  b_lex_states[B_LEX_STATES_STATE_STR].func = b_lex_state_str;
  b_lex_states[B_LEX_STATES_STATE_STR].sz = B_LEX_STATE_STR_max;

  /* zero next states */
  for (i = 0; i < B_LEX_STATES_STATE_max; i++) {
    b_lex_states[i].next = calloc(sizeof(struct b_ndfa *), b_lex_states[i].sz);
    assert(b_lex_states[i].next);
  }

  /* mappinb_lex_statess */
  b_lex_states[B_LEX_STATES_STATE_EXPR].next[B_LEX_STATE_EXPR_LETTER] =
      &b_lex_states[B_LEX_STATES_STATE_IDENT];
  b_lex_states[B_LEX_STATES_STATE_EXPR].next[B_LEX_STATE_EXPR_DIGIT] =
      &b_lex_states[B_LEX_STATES_STATE_NUMBER];
  b_lex_states[B_LEX_STATES_STATE_EXPR].next[B_LEX_STATE_EXPR_WHITESPACE] =
      &b_lex_states[B_LEX_STATES_STATE_WHITESPACE1];
  b_lex_states[B_LEX_STATES_STATE_EXPR].next[B_LEX_STATE_EXPR_LPAREN] =
      &b_lex_states[B_LEX_STATES_STATE_TUPLE];
  b_lex_states[B_LEX_STATES_STATE_EXPR].next[B_LEX_STATE_EXPR_RPAREN] =
      &b_lex_states[B_LEX_STATES_STATE_OPERATOR];
  b_lex_states[B_LEX_STATES_STATE_EXPR].next[B_LEX_STATE_EXPR_QUOTE] =
      &b_lex_states[B_LEX_STATES_STATE_STR];
  b_lex_states[B_LEX_STATES_STATE_OPERATOR]
      .next[B_LEX_STATE_OPERATOR_WHITESPACE] =
      &b_lex_states[B_LEX_STATES_STATE_WHITESPACE2];
  b_lex_states[B_LEX_STATES_STATE_OPERATOR].next[B_LEX_STATE_OPERATOR_COMMA] =
      &b_lex_states[B_LEX_STATES_STATE_EXPR];
  b_lex_states[B_LEX_STATES_STATE_OPERATOR].next[B_LEX_STATE_OPERATOR_DOT] =
      &b_lex_states[B_LEX_STATES_STATE_EXPR];
  b_lex_states[B_LEX_STATES_STATE_OPERATOR].next[B_LEX_STATE_OPERATOR_LPAREN] =
      &b_lex_states[B_LEX_STATES_STATE_EXPR];
  b_lex_states[B_LEX_STATES_STATE_OPERATOR].next[B_LEX_STATE_OPERATOR_RPAREN] =
      &b_lex_states[B_LEX_STATES_STATE_OPERATOR];
  b_lex_states[B_LEX_STATES_STATE_TUPLE].next[B_LEX_STATE_TUPLE_TUPLE] =
      &b_lex_states[B_LEX_STATES_STATE_EXPR];
  b_lex_states[B_LEX_STATES_STATE_STR].next[B_LEX_STATE_STR_STR] =
      &b_lex_states[B_LEX_STATES_STATE_OPERATOR];
  b_lex_states[B_LEX_STATES_STATE_IDENT].next[B_LEX_STATE_IDENT_IDENT] =
      &b_lex_states[B_LEX_STATES_STATE_OPERATOR];
  b_lex_states[B_LEX_STATES_STATE_NUMBER].next[B_LEX_STATE_NUMBER_NUMBER] =
      &b_lex_states[B_LEX_STATES_STATE_OPERATOR];
  b_lex_states[B_LEX_STATES_STATE_WHITESPACE1]
      .next[B_LEX_STATE_WHITESPACE_WHITESPACE] =
      &b_lex_states[B_LEX_STATES_STATE_EXPR];
  b_lex_states[B_LEX_STATES_STATE_WHITESPACE2]
      .next[B_LEX_STATE_WHITESPACE_WHITESPACE] =
      &b_lex_states[B_LEX_STATES_STATE_OPERATOR];
}

/* creates new lexer */
int b_new_lexer(struct b_lex **l) {
  *l = calloc(sizeof(struct b_lex), 1);
  if (!*l) {
    return -1;
  }
  return 0;
}
