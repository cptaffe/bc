/* Copyright 2016 Connor Taffe */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "b.h"

int main(int argc __attribute__((unused)),
         char *argv[] __attribute__((unused))) {
  struct b_ndfa g[9];
  struct b_lex l;
  struct b_token_list *tok;
  size_t i;
  int ret;

  ret = 1;
  memset(g, 0, sizeof(g));
  memset(&l, 0, sizeof(l));

  for (i = 0; i < sizeof(g) / sizeof(struct b_ndfa); i++) {
    g[i].sz = 4; /* not guaranteed to be constant */
    g[i].next = calloc(sizeof(struct b_ndfa *), g[i].sz);
    if (!g[i].next)
      goto failure;
  }

  /* functions */
  g[0].func = b_lex_state_start;
  g[1].func = b_lex_state_ident;
  g[2].func = b_lex_state_function_call;
  g[3].func = b_lex_state_function_call_args;
  g[4].func = b_lex_state_whitespace;
  g[5].func = b_lex_state_whitespace;
  g[6].func = b_lex_state_ident;
  g[7].func = b_lex_state_function_call_args_post;
  g[8].func = b_lex_state_whitespace;

  /* mappings */
  g[0].next[0] = &g[1]; /* start -> func ident */
  g[0].next[1] = &g[4]; /* start -> whitespace */
  g[4].next[0] = &g[0]; /* whitespace -> start */
  g[1].next[0] = &g[2]; /* ident -> func call */
  g[2].next[0] = &g[3]; /* func call -> func call args */
  g[3].next[0] = &g[5]; /* func call args -> whitespace */
  g[3].next[1] = &g[0]; /* func call args -> start */
  g[3].next[2] = &g[6]; /* func call args -> ident */
  g[5].next[0] = &g[3]; /* whitespace -> function call args */
  g[6].next[0] = &g[7]; /* ident -> function call args post */
  g[7].next[0] = &g[8]; /* func call args post -> whitespace */
  g[7].next[1] = &g[0]; /* func call args post -> start */
  g[7].next[2] = &g[6]; /* func call args post -> ident */
  g[7].next[3] = &g[7]; /* func call args post -> func call args post */
  g[8].next[0] = &g[7]; /* whitespace -> func call args post */

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
