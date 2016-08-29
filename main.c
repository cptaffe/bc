/* Copyright 2016 Connor Taffe */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "b.h"

struct b_tree {
  struct b_token *tok;
  struct b_tree *parent;
  struct b_tree **child;
  size_t sz, l;
};

int b_fmt(struct b_token *tok, char **buf);
int b_parse(struct b_tree *t, struct b_token_list *l);
int b_tree_append(struct b_tree *t, struct b_tree *c);
int b_fmt_tree(struct b_tree *t);

/* format token */
int b_fmt(struct b_token *tok, char **buf) {
  char *fmt, *ffmt;
  int sz;
  int ret;

  ret = -1;
  fmt = 0;
  ffmt = "%%s:%%d#%%d-> %%d; '%%.%lds'\n";

  assert(tok);
  assert(buf);

  sz = snprintf(0, 0, ffmt, tok->sz) + 1;
  if (sz < 0)
    goto failure;
  fmt = calloc(sizeof(char), (size_t)sz);
  if (!fmt)
    goto failure;
  snprintf(fmt, (size_t)sz, ffmt, tok->sz);

  sz = snprintf(0, 0, fmt, "stdin", tok->line + 1, tok->col + 1, tok->type,
                tok->buf);
  if (sz < 0)
    goto failure;
  *buf = calloc(sizeof(char), (size_t)sz);
  if (!*buf)
    goto failure;
  snprintf(*buf, (size_t)sz, fmt, "stdin", tok->line + 1, tok->col + 1,
           tok->type, tok->buf);

  ret = 0;
failure:
  free(fmt);
  return ret;
}

int b_tree_append(struct b_tree *t, struct b_tree *c) {
  if (t->l == t->sz) {
    t->sz = t->sz * 2 + 1;
    t->child = realloc(t->child, sizeof(struct b_tree *) * t->sz);
    if (!t->child)
      return -1;
  }
  t->child[t->l++] = c;
  c->parent = t;
  return 0;
}

/*
  dot is infix,
  <operand>.<operand>
  parenthesis are postfix or for grouping.
  <operand>?(); a().().b
  commas are infix,
  <operand>,<operand>
*/
int b_parse(struct b_tree *r, struct b_token_list *ls) {
  char *s;

  assert(ls);
  assert(r);

  for (; ls; ls = ls->next) {
    s = 0;
    if (b_fmt(&ls->tok, &s) != -1)
      puts(s);
    free(s);
  }
  return 0;
}

int b_fmt_tree(struct b_tree *t) {
  char *s;
  size_t i;

  assert(t);
  assert(t->tok);

  s = 0;

  if (b_fmt(t->tok, &s) != -1)
    puts(s);
  free(s);

  puts("(");

  for (i = 0; i < t->l; i++) {
    s = 0;
    if (b_fmt_tree(t->child[i]) != -1)
      puts(s);
    free(s);
  }

  puts(")");
  return 0;
}

int main(int argc __attribute__((unused)),
         char *argv[] __attribute__((unused))) {
  struct b_lex l;
  struct b_tree t;

  memset(&l, 0, sizeof(l));
  memset(&t, 0, sizeof(t));

  /* parse input */
  b_state_machine(&l, b_lex_states);
  if (b_parse(&t, l.head) == -1)
    return 1;

  if (t.tok)
    b_fmt_tree(&t);

  exit(0);
}
