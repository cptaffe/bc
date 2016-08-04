/* Copyright 2016 Connor Taffe */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "b.h"

int main(int argc __attribute__((unused)),
         char *argv[] __attribute__((unused))) {
  struct b_ndfa g[6];
  struct b_lex l;
  struct b_token_list *tok;
  size_t i;
  int ret;

  ret = 1;
  memset(g, 0, sizeof(g));
  memset(&l, 0, sizeof(l));

  for (i = 0; i < sizeof(g) / sizeof(struct b_ndfa); i++) {
    g[i].sz = 5; /* not guaranteed to be constant */
    g[i].next = calloc(sizeof(struct b_ndfa *), g[i].sz);
    if (!g[i].next)
      goto failure;
  }

  /* functions */
  g[0].func = b_lex_state_expr;
  g[1].func = b_lex_state_ident;
  g[2].func = b_lex_state_whitespace;
  g[3].func = b_lex_state_whitespace;
  g[4].func = b_lex_state_operator;
  g[5].func = b_lex_state_number;

  /* mappings */
  g[0].next[0] = &g[1];                /* expr -> ident */
  g[0].next[1] = &g[5];                /* expr -> ident */
  g[0].next[2] = &g[3];                /* expr -> whitespace */
  g[0].next[3] = &g[4];                /* expr -> oper */
  g[3].next[0] = &g[0];                /* whitespace -> expr */
  g[1].next[0] = &g[4];                /* ident -> oper */
  g[5].next[0] = &g[4];                /* number literal -> oper */
  g[4].next[0] = &g[2];                /* oper -> whitespace */
  g[2].next[0] = &g[4];                /* whitespace -> oper */
  g[4].next[1] = &g[1];                /* oper -> ident */
  g[4].next[2] = g[4].next[3] = &g[0]; /* oper -> expr */
  g[4].next[4] = &g[4];                /* oper -> oper */

  /* parse input */
  b_state_machine(&l, g);

  for (tok = l.list; tok; tok = tok->next) {
    char *fmt, *ffmt;
    int sz;
    ffmt = "tok: %%d; '%%.%lds'\n";
    sz = snprintf(0, 0, ffmt, tok->tok.sz) + 1;
    if (sz < 0)
      goto failure;
    fmt = calloc(sizeof(char), (size_t)sz);
    snprintf(fmt, (size_t)sz, ffmt, tok->tok.sz);
    printf(fmt, tok->tok.type, tok->tok.buf);
    free(fmt);
  }

  ret = 0;
failure:
  for (i = 0; i < sizeof(g) / sizeof(struct b_ndfa); i++)
    free(g[i].next);
  exit(ret);
}
