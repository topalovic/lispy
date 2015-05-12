#ifndef builtin_h
#define builtin_h

#include "lval.h"

LVAL* builtin_var(LENV* e, LVAL* a, char* func);
LVAL* builtin_lambda(LENV* e, LVAL* a);
LVAL* builtin_def(LENV* e, LVAL* a);
LVAL* builtin_put(LENV* e, LVAL* a);

LVAL* builtin_eval(LENV *e, LVAL* a);
LVAL* builtin_list(LENV *e, LVAL* a);
LVAL* builtin_len(LENV *e, LVAL* a);
LVAL* builtin_head(LENV *e, LVAL* a);
LVAL* builtin_tail(LENV *e, LVAL* a);
LVAL* builtin_join(LENV *e, LVAL* a);
LVAL* builtin_cons(LENV *e, LVAL *a);

LVAL* builtin_op(LVAL* a, char* op);
LVAL* builtin_add(LENV* e, LVAL* a);
LVAL* builtin_sub(LENV* e, LVAL* a);
LVAL* builtin_mul(LENV* e, LVAL* a);
LVAL* builtin_div(LENV* e, LVAL* a);
LVAL* builtin_mod(LENV* e, LVAL* a);

LVAL* builtin_ord(LENV* e, LVAL* a, char* op);
LVAL* builtin_gt(LENV* e, LVAL* a);
LVAL* builtin_lt(LENV* e, LVAL* a);
LVAL* builtin_ge(LENV* e, LVAL* a);
LVAL* builtin_le(LENV* e, LVAL* a);

LVAL* builtin_cmp(LENV* e, LVAL* a, char* op);
LVAL* builtin_eq(LENV* e, LVAL* a);
LVAL* builtin_ne(LENV* e, LVAL* a);
LVAL* builtin_if(LENV* e, LVAL* a);

LVAL* builtin_type(LENV* e, LVAL* a);
LVAL* builtin_print(LENV* e, LVAL* a);
LVAL* builtin_error(LENV* e, LVAL* a);

void lenv_register_builtin(LENV* e, char* name, LBUILTIN func);
void lenv_register_builtins(LENV* e);

#endif
