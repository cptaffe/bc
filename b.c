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

      b_lex_back(l);

      if (!b_lex_len(l)) {
        tok.type = B_TOK_ERR;
        tok.buf = "Expected identifier";
        tok.sz = strlen(tok.buf);
        b_lex_emit(l, tok);
        return 1;
      }

      tok.type = B_TOK_IDENT;
      b_lex_buf(l, &tok.buf, &tok.sz);
      b_lex_emit(l, tok);

      return 0;
    }
  }
}

/* operator */
int b_lex_state_operator(struct b_lex *l) {
  char c;
  struct b_token tok;

  assert(l);

  for (;;) {
    if (b_lex_next(l, &c) == -1)
      return -1;
    if (b_character_is_whitespace(c))
      return 0;
    else if (c == '.') {
      /* emit dot operator */

      tok.type = B_TOK_OPER;
      b_lex_buf(l, &tok.buf, &tok.sz);
      b_lex_emit(l, tok);

      return 1;
    } else if (c == ',') {

      tok.type = B_TOK_COMMA;
      b_lex_buf(l, &tok.buf, &tok.sz);
      b_lex_emit(l, tok);

      return 2;
    } else if (c == '(') {
      /* emit opening paren */

      l->pdepth++;

      tok.type = B_TOK_LPAREN;
      b_lex_buf(l, &tok.buf, &tok.sz);
      b_lex_emit(l, tok);

      return 3;
    } else if (c == ')') {
      /* emit right parenthesis */

      if (l->pdepth > 0) {
        l->pdepth--;

        tok.type = B_TOK_RPAREN;
        b_lex_buf(l, &tok.buf, &tok.sz);
        b_lex_emit(l, tok);

        return 4;
      } else {
        /* error */
        char *buf = "Too many closing parens.";

        tok.type = B_TOK_ERR;
        tok.buf = buf;
        tok.sz = strlen(buf);
        b_lex_emit(l, tok);
        return 5;
      }
    } else if (b_character_is_letter(c))
      return b_lex_back(l), 6;
  }
}

/* type literal */
int b_lex_state_number(struct b_lex *l) {
  char c;

  assert(l);

  for (;;) {
    if (b_lex_next(l, &c) == -1)
      return -1;
    if (b_character_is_digit(c))
      ;
    else {
      /* emit ident */
      struct b_token tok;

      b_lex_back(l);

      if (!b_lex_len(l)) {
        tok.type = B_TOK_ERR;
        tok.buf = "Expected identifier";
        tok.sz = strlen(tok.buf);
        b_lex_emit(l, tok);
        return 1;
      }

      tok.type = B_TOK_NUMBER;
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

int b_lex_state_expr(struct b_lex *l) {
  char c;

  assert(l);

  for (;;) {
    if (b_lex_next(l, &c) == -1)
      return -1;
    if (b_character_is_letter(c))
      return b_lex_back(l), 0;
    else if (b_character_is_digit(c))
      return b_lex_back(l), 1;
    else if (b_character_is_whitespace(c))
      return b_lex_back(l), 2;
    else if (c == ')')
      return b_lex_back(l), 3;
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
