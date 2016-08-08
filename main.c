/* Copyright 2016 Connor Taffe */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "b.h"

static struct b_ndfa b_ndfa_expr, b_ndfa_ident, b_ndfa_whitespace,
    b_ndfa_operator, b_ndfa_number, b_ndfa_tuple, b_ndfa_str;

__attribute__((constructor)) void b_ndfa_init(void);

void b_ndfa_init() {
  /* epression */
  memset(&b_ndfa_expr, 0, sizeof(struct b_ndfa));
  b_ndfa_expr.func = b_lex_state_expr;
  b_ndfa_expr.sz = B_LEX_STATE_EXPR_max;

  memset(&b_ndfa_ident, 0, sizeof(struct b_ndfa));
  b_ndfa_ident.func = b_lex_state_ident;
  b_ndfa_ident.sz = B_LEX_STATE_IDENT_max;

  memset(&b_ndfa_whitespace, 0, sizeof(struct b_ndfa));
  b_ndfa_whitespace.func = b_lex_state_whitespace;
  b_ndfa_whitespace.sz = B_LEX_STATE_WHITESPACE_max;

  memset(&b_ndfa_operator, 0, sizeof(struct b_ndfa));
  b_ndfa_operator.func = b_lex_state_operator;
  b_ndfa_operator.sz = B_LEX_STATE_OPERATOR_max;

  memset(&b_ndfa_number, 0, sizeof(struct b_ndfa));
  b_ndfa_number.func = b_lex_state_number;
  b_ndfa_number.sz = B_LEX_STATE_NUMBER_max;

  memset(&b_ndfa_tuple, 0, sizeof(struct b_ndfa));
  b_ndfa_tuple.func = b_lex_state_tuple;
  b_ndfa_tuple.sz = B_LEX_STATE_TUPLE_max;

  memset(&b_ndfa_str, 0, sizeof(struct b_ndfa));
  b_ndfa_str.func = b_lex_state_str;
  b_ndfa_str.sz = B_LEX_STATE_STR_max;
}

int main(int argc __attribute__((unused)),
         char *argv[] __attribute__((unused))) {
  struct b_ndfa *g;
  struct b_lex l;
  struct b_token_list *tok;
  size_t i;
  int ret;

  enum {
    NDFA_EXPR,
    NDFA_IDENT,
    NDFA_WHITESPACE1,
    NDFA_WHITESPACE2,
    NDFA_OPERATOR,
    NDFA_NUMBER,
    NDFA_TUPLE,
    NDFA_STR,
    NDFA_max
  };

  ret = 1;
  memset(&l, 0, sizeof(l));

  g = calloc(sizeof(struct b_ndfa), NDFA_max);
  if (!g)
    goto failure;

  g[NDFA_EXPR] = b_ndfa_expr;
  g[NDFA_IDENT] = b_ndfa_ident;
  g[NDFA_WHITESPACE1] = b_ndfa_whitespace;
  g[NDFA_WHITESPACE2] = b_ndfa_whitespace;
  g[NDFA_OPERATOR] = b_ndfa_operator;
  g[NDFA_NUMBER] = b_ndfa_number;
  g[NDFA_TUPLE] = b_ndfa_tuple;
  g[NDFA_STR] = b_ndfa_str;

  for (i = 0; i < NDFA_max; i++) {
    g[i].next = calloc(sizeof(struct b_ndfa *), g[i].sz);
    if (!g[i].next)
      goto failure;
  }

  /* mappings */
  g[NDFA_EXPR].next[B_LEX_STATE_EXPR_LETTER] = &g[NDFA_IDENT];
  g[NDFA_EXPR].next[B_LEX_STATE_EXPR_DIGIT] = &g[NDFA_NUMBER];
  g[NDFA_EXPR].next[B_LEX_STATE_EXPR_WHITESPACE] = &g[NDFA_WHITESPACE1];
  g[NDFA_EXPR].next[B_LEX_STATE_EXPR_LPAREN] = &g[NDFA_TUPLE];
  g[NDFA_EXPR].next[B_LEX_STATE_EXPR_RPAREN] = &g[NDFA_OPERATOR];
  g[NDFA_EXPR].next[B_LEX_STATE_EXPR_QUOTE] = &g[NDFA_STR];
  g[NDFA_OPERATOR].next[B_LEX_STATE_OPERATOR_WHITESPACE] = &g[NDFA_WHITESPACE2];
  g[NDFA_OPERATOR].next[B_LEX_STATE_OPERATOR_COMMA] = &g[NDFA_EXPR];
  g[NDFA_OPERATOR].next[B_LEX_STATE_OPERATOR_DOT] = &g[NDFA_EXPR];
  g[NDFA_OPERATOR].next[B_LEX_STATE_OPERATOR_LPAREN] = &g[NDFA_EXPR];
  g[NDFA_OPERATOR].next[B_LEX_STATE_OPERATOR_RPAREN] = &g[NDFA_OPERATOR];
  g[NDFA_TUPLE].next[B_LEX_STATE_TUPLE_TUPLE] = &g[NDFA_EXPR];
  g[NDFA_STR].next[B_LEX_STATE_STR_STR] = &g[NDFA_OPERATOR];
  g[NDFA_IDENT].next[B_LEX_STATE_IDENT_IDENT] = &g[NDFA_OPERATOR];
  g[NDFA_NUMBER].next[B_LEX_STATE_NUMBER_NUMBER] = &g[NDFA_OPERATOR];
  g[NDFA_WHITESPACE1].next[B_LEX_STATE_WHITESPACE_WHITESPACE] = &g[NDFA_EXPR];
  g[NDFA_WHITESPACE2].next[B_LEX_STATE_WHITESPACE_WHITESPACE] =
      &g[NDFA_OPERATOR];

  /* parse input */
  b_state_machine(&l, g);

  for (tok = l.list; tok; tok = tok->next) {
    char *fmt, *ffmt;
    int sz;
    ffmt = "%%s:%%d#%%d-> %%d; '%%.%lds'\n";
    sz = snprintf(0, 0, ffmt, tok->tok.sz) + 1;
    if (sz < 0)
      goto failure;
    fmt = calloc(sizeof(char), (size_t)sz);
    snprintf(fmt, (size_t)sz, ffmt, tok->tok.sz);
    printf(fmt, "stdin", tok->tok.line + 1, tok->tok.col + 1, tok->tok.type,
           tok->tok.buf);
    free(fmt);
  }

  ret = 0;
failure:
  if (g)
    for (i = 0; i < NDFA_max; i++)
      free(g[i].next);
  free(g);
  exit(ret);
}
