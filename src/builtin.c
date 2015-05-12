#include "builtin.h"

/* Builtins */

LVAL* builtin_var(LENV* e, LVAL* a, char* func) {
    LASSERT_TYPE(func, a, 0, LVAL_QEXPR);

    LVAL* syms = a->cell[0];
    for (int i = 0; i < syms->count; i++) {
        LASSERT(a, (syms->cell[i]->type == LVAL_SYM),
                "Function '%s' cannot define non-symbol. "
                "Got %s, expected %s.", func,
                ltype_name(syms->cell[i]->type),
                ltype_name(LVAL_SYM));
    }

    LASSERT(a, (syms->count == a->count-1),
            "Function '%s' passed too many arguments for symbols. "
            "Got %i, expected %i.", func, syms->count, a->count-1);

    for (int i = 0; i < syms->count; i++) {
        /* If 'def' define in globally. If 'put' define in locally */
        if (strcmp(func, "def") == 0) {
            lenv_def(e, syms->cell[i], a->cell[i+1]);
        }

        if (strcmp(func, "=")   == 0) {
            lenv_put(e, syms->cell[i], a->cell[i+1]);
        }
    }

    lval_del(a);
    return lval_sexpr();
}

LVAL* builtin_lambda(LENV* e, LVAL* a) {
    LASSERT_NUM("->", a, 2);
    LASSERT_TYPE("->", a, 0, LVAL_QEXPR);
    LASSERT_TYPE("->", a, 1, LVAL_QEXPR);

    /* First q-expression should contain only symbols */
    for (int i = 0; i < a->cell[0]->count; i++) {
        LASSERT(a, (a->cell[0]->cell[i]->type == LVAL_SYM),
                "Cannot define non-symbol. Got %s, expected %s.",
                ltype_name(a->cell[0]->cell[i]->type), ltype_name(LVAL_SYM));
    }

    LVAL* formals = lval_pop(a, 0);
    LVAL* body = lval_pop(a, 0);
    lval_del(a);

    return lval_lambda(formals, body);
}

LVAL* builtin_def(LENV* e, LVAL* a) {
    return builtin_var(e, a, "def");
}

LVAL* builtin_put(LENV* e, LVAL* a) {
    return builtin_var(e, a, "=");
}

LVAL* builtin_eval(LENV *e, LVAL* a) {
    LASSERT_NUM("eval", a, 1);
    LASSERT_TYPE("eval", a, 0, LVAL_QEXPR);

    LVAL* x = lval_take(a, 0);
    x->type = LVAL_SEXPR;
    return lval_eval(e, x);
}

LVAL* builtin_list(LENV *e, LVAL* a) {
    a->type = LVAL_QEXPR;
    return a;
}

LVAL* builtin_len(LENV *e, LVAL* a) {
    LASSERT_NUM("len", a, 1);
    LASSERT_TYPE("len", a, 0, LVAL_QEXPR);

    LVAL *x = lval_num(a->cell[0]->count);
    lval_del(a);
    return x;
}

LVAL* builtin_head(LENV *e, LVAL* a) {
    LASSERT_NUM("head", a, 1);
    LASSERT_TYPE("head", a, 0, LVAL_QEXPR);
    LASSERT_NOT_EMPTY("head", a, 0);

    LVAL* v = lval_take(a, 0);
    while (v->count > 1) { lval_del(lval_pop(v, 1)); }
    return v;
}

LVAL* builtin_tail(LENV *e, LVAL* a) {
    LASSERT_NUM("tail", a, 1);
    LASSERT_TYPE("tail", a, 0, LVAL_QEXPR);
    LASSERT_NOT_EMPTY("tail", a, 0);

    LVAL* v = lval_take(a, 0);
    lval_del(lval_pop(v, 0));
    return v;
}

LVAL* builtin_join(LENV *e, LVAL* a) {
    for (int i = 0; i < a->count; i++) {
        LASSERT_TYPE("join", a, i, LVAL_QEXPR);
    }

    LVAL* x = lval_pop(a, 0);

    while (a->count) {
        x = lval_join(x, lval_pop(a, 0));
    }

    lval_del(a);
    return x;
}

LVAL* builtin_cons(LENV *e, LVAL *a) {
    LASSERT_NUM("cons", a, 2);
    LASSERT_TYPE("cons", a, 1, LVAL_QEXPR);

    LVAL* x = lval_qexpr();

    x = lval_add(x, lval_pop(a, 0));
    x = lval_join(x, lval_pop(a, 0));

    lval_del(a);
    return x;
}

LVAL* builtin_op(LVAL* a, char* op) {
    /* Ensure all arguments are numbers */
    for (int i = 0; i < a->count; i++) {
        LASSERT_TYPE(op, a, i, LVAL_NUM);
    }

    /* Pop the first element */
    LVAL* x = lval_pop(a, 0);

    /* If no arguments and sub then perform unary negation */
    if ((strcmp(op, "-") == 0) && a->count == 0) {
        x->num = -x->num;
    }

    /* While there are still elements remaining */
    while (a->count > 0) {
        LVAL* y = lval_pop(a, 0);

        if ((strcmp(op, "/") == 0) || (strcmp(op, "%") == 0)) {
            if (y->num == 0) {
                lval_del(x);
                lval_del(y);
                x = lval_err("Division by zero"); break;
            }
        }

        if (strcmp(op, "+") == 0) { x->num += y->num; }
        if (strcmp(op, "-") == 0) { x->num -= y->num; }
        if (strcmp(op, "*") == 0) { x->num *= y->num; }
        if (strcmp(op, "/") == 0) { x->num /= y->num; }
        if (strcmp(op, "%") == 0) { x->num %= y->num; }

        lval_del(y);
    }

    lval_del(a);
    return x;
}

LVAL* builtin_add(LENV* e, LVAL* a) {
    return builtin_op(a, "+");
}

LVAL* builtin_sub(LENV* e, LVAL* a) {
    return builtin_op(a, "-");
}

LVAL* builtin_mul(LENV* e, LVAL* a) {
    return builtin_op(a, "*");
}

LVAL* builtin_div(LENV* e, LVAL* a) {
    return builtin_op(a, "/");
}

LVAL* builtin_mod(LENV* e, LVAL* a) {
    return builtin_op(a, "%");
}

LVAL* builtin_ord(LENV* e, LVAL* a, char* op) {
    LASSERT_NUM(op, a, 2);
    LASSERT_TYPE(op, a, 0, LVAL_NUM);
    LASSERT_TYPE(op, a, 1, LVAL_NUM);

    int r;
    if (strcmp(op, ">")  == 0) {
        r = (a->cell[0]->num > a->cell[1]->num);
    }
    if (strcmp(op, "<")  == 0) {
        r = (a->cell[0]->num < a->cell[1]->num);
    }
    if (strcmp(op, ">=") == 0) {
        r = (a->cell[0]->num >= a->cell[1]->num);
    }
    if (strcmp(op, "<=") == 0) {
        r = (a->cell[0]->num <= a->cell[1]->num);
    }
    lval_del(a);
    return lval_num(r);
}

LVAL* builtin_gt(LENV* e, LVAL* a) {
    return builtin_ord(e, a, ">");
}

LVAL* builtin_lt(LENV* e, LVAL* a) {
    return builtin_ord(e, a, "<");
}

LVAL* builtin_ge(LENV* e, LVAL* a) {
    return builtin_ord(e, a, ">=");
}

LVAL* builtin_le(LENV* e, LVAL* a) {
    return builtin_ord(e, a, "<=");
}

LVAL* builtin_cmp(LENV* e, LVAL* a, char* op) {
    LASSERT_NUM(op, a, 2);
    int r;
    if (strcmp(op, "==") == 0) {
        r =  lval_eq(a->cell[0], a->cell[1]);
    }
    if (strcmp(op, "!=") == 0) {
        r = !lval_eq(a->cell[0], a->cell[1]);
    }
    lval_del(a);
    return lval_num(r);
}

LVAL* builtin_eq(LENV* e, LVAL* a) {
    return builtin_cmp(e, a, "==");
}

LVAL* builtin_ne(LENV* e, LVAL* a) {
    return builtin_cmp(e, a, "!=");
}

LVAL* builtin_if(LENV* e, LVAL* a) {
    LASSERT_NUM("if", a, 3);
    LASSERT_TYPE("if", a, 0, LVAL_NUM);
    LASSERT_TYPE("if", a, 1, LVAL_QEXPR);
    LASSERT_TYPE("if", a, 2, LVAL_QEXPR);

    LVAL* x;
    int cond = a->cell[0]->num;

    /* Mark both expressions as evaluable */
    a->cell[1]->type = LVAL_SEXPR;
    a->cell[2]->type = LVAL_SEXPR;

    x = cond ? lval_eval(e, lval_pop(a, 1))
             : lval_eval(e, lval_pop(a, 2));

    lval_del(a);
    return x;
}

LVAL* builtin_type(LENV* e, LVAL* a) {
    char *s = ltype_name(a->cell[0]->type);
    lval_del(a);
    return lval_str(s);
}

LVAL* builtin_print(LENV* e, LVAL* a) {
    for (int i = 0; i < a->count; i++) {
        lval_print(a->cell[i]); putchar(' ');
    }

    putchar('\n');
    lval_del(a);

    return lval_sexpr();
}

LVAL* builtin_error(LENV* e, LVAL* a) {
    LASSERT_NUM("error", a, 1);
    LASSERT_TYPE("error", a, 0, LVAL_STR);

    LVAL* err = lval_err(a->cell[0]->str);

    lval_del(a);
    return err;
}

void lenv_register_builtin(LENV* e, char* name, LBUILTIN func) {
    LVAL* k = lval_sym(name);
    LVAL* v = lval_fun(func, name);
    lenv_put(e, k, v);
    lval_del(k);
    lval_del(v);
}
