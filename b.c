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
    return 0;
  }
  return -1;
}

void b_lex_back(struct b_lex *l) {
  assert(l != 0);

  if (l->i > l->l)
    l->i--;
}

void b_lex_emit(struct b_lex *l, struct b_token tok) {
  struct b_token_list *link;

  assert(l != 0);

  link = calloc(sizeof(struct b_token_list), 1);
  assert(link != 0);
  link->tok = tok;
  link->next = l->list;
  l->list = link;
}

/* gets & resets buffer */
void b_lex_buf(struct b_lex *l, char **buf, size_t *sz) {
  assert(l != 0);
  assert(buf != 0);
  assert(sz != 0);

  if (l->i == l->l)
    return;

  *sz = l->i - l->l;
  *buf = calloc(sizeof(char), *sz);
  assert(buf != 0);
  memcpy(*buf, &l->message[l->l], *sz);
  l->l = l->i; /* reset buffer */
}

int b_state_machine(struct b_lex *l, struct b_ndfa *g) {
  int next;

  for (; g && (size_t)(next = g->func(l)) < g->sz; g = g->next[next])
    ;

  return -1;
}

bool b_character_is_letter(char c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

bool b_character_is_whitespace(char c) {
  return c == ' ' || c == '\t' || c == '\n';
}

int b_lex_state_function_call_args_base(struct b_lex *l, bool post_ident);

int b_lex_state_function_call_args(struct b_lex *l) {
  return b_lex_state_function_call_args_base(l, false);
}

int b_lex_state_function_call_args_post(struct b_lex *l) {
  return b_lex_state_function_call_args_base(l, true);
}

int b_lex_state_function_call_args_base(struct b_lex *l, bool post_ident) {
  char c;

  assert(l);

  for (;;) {
    if (b_lex_next(l, &c) == -1)
      return -1;
    if (b_character_is_whitespace(c))
      return 0;
    else if (c == ')') {
      /* emit right parenthesis */
      struct b_token tok;

      tok.type = B_TOK_RPAREN;
      b_lex_buf(l, &tok.buf, &tok.sz);
      b_lex_emit(l, tok);

      return 1;
    } else if (b_character_is_letter(c))
      return 2;
    else if (c == ',') {
      struct b_token tok;
      if (!post_ident) {
        /* error, empty parameter not allowed */
        char *buf = "Empty parameter not allowed.";

        tok.type = B_TOK_ERR;
        tok.buf = buf;
        tok.sz = strlen(buf);
        b_lex_emit(l, tok);
      } else {
        tok.type = B_TOK_COMMA;
        b_lex_buf(l, &tok.buf, &tok.sz);
        b_lex_emit(l, tok);
      }

      return 3;
    } else {
      /* error */
      char *buf, *fmt;
      struct b_token tok;
      int sz;

      fmt = "Unexpected character '%c', expected function parameter.";

      sz = snprintf(0, 0, fmt, c) + 1;
      if (sz < 0)
        return -1;
      buf = calloc(sizeof(char), (size_t)sz);
      if (!buf)
        return -1;
      snprintf(buf, (size_t)sz, fmt, c);

      tok.type = B_TOK_ERR;
      tok.buf = buf;
      tok.sz = (size_t)sz;
      b_lex_emit(l, tok);

      return 3;
    }
  }
}

int b_lex_state_function_call(struct b_lex *l) {
  char c;

  assert(l);

  for (;;) {
    if (b_lex_next(l, &c) == -1)
      return -1;
    if (b_character_is_whitespace(c))
      ;
    else if (c == '(') {
      /* emit opening paren */
      struct b_token tok;

      tok.type = B_TOK_LPAREN;
      b_lex_buf(l, &tok.buf, &tok.sz);
      b_lex_emit(l, tok);

      return 0;
    } else {
      /* error */
      char *buf, *fmt;
      struct b_token tok;
      int sz;

      fmt = "Unexpected character '%c', expected function call.";
      sz = snprintf(0, 0, fmt, c) + 1;
      if (sz < 0)
        return -1;
      buf = calloc(sizeof(char), (size_t)sz);
      snprintf(buf, (size_t)sz, fmt, c);

      tok.type = B_TOK_ERR;
      tok.buf = buf;
      tok.sz = (size_t)sz;
      b_lex_emit(l, tok);

      return 1;
    }
  }
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

      b_lex_back(l);

      tok.type = B_TOK_IDENT;
      b_lex_buf(l, &tok.buf, &tok.sz);
      b_lex_emit(l, tok);

      return 0;
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

      b_lex_back(l);

      tok.type = B_TOK_WHITESPACE;
      b_lex_buf(l, &tok.buf, &tok.sz);
      b_lex_emit(l, tok);

      return 0;
    }
  }
}

int b_lex_state_start(struct b_lex *l) {
  char c;

  assert(l);

  for (;;) {
    if (b_lex_next(l, &c) == -1)
      return -1;
    if (b_character_is_letter(c))
      return b_lex_back(l), 0;
    else if (b_character_is_whitespace(c))
      return b_lex_back(l), 1;
    else {
      /* error */
      char *buf, *fmt;
      struct b_token tok;
      int sz;

      fmt = "Unexpected character '%c', expected identifier.";
      sz = snprintf(0, 0, fmt, c);
      if (sz < 0)
        return -1;
      buf = calloc(sizeof(char), (size_t)sz);
      snprintf(buf, (size_t)sz, fmt, c);

      tok.type = B_TOK_ERR;
      tok.buf = buf;
      tok.sz = (size_t)sz;
      b_lex_emit(l, tok);

      return 2;
    }
  }
}