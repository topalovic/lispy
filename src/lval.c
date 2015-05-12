#include "lval.h"

/* Lisp values */

char* ltype_name(int t) {
    switch(t) {
    case LVAL_FUN: return "function";
    case LVAL_NUM: return "number";
    case LVAL_ERR: return "error";
    case LVAL_SYM: return "symbol";
    case LVAL_STR: return "string";
    case LVAL_SEXPR: return "sexpr";
    case LVAL_QEXPR: return "qexpr";
    default: return "unknown";
    }
}

/* LVAL constructors */

LVAL* lval_num(long x) {
    LVAL* v = malloc(sizeof(LVAL));
    v->type = LVAL_NUM;
    v->num = x;
    return v;
}

LVAL* lval_sym(char* s) {
    LVAL* v = malloc(sizeof(LVAL));
    v->type = LVAL_SYM;
    v->sym = malloc(strlen(s) + 1);
    strcpy(v->sym, s);
    return v;
}

LVAL* lval_str(char* s) {
    LVAL* v = malloc(sizeof(LVAL));
    v->type = LVAL_STR;
    v->str = malloc(strlen(s) + 1);
    strcpy(v->str, s);
    return v;
}

LVAL* lval_sexpr(void) {
    LVAL* v = malloc(sizeof(LVAL));
    v->type = LVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

LVAL* lval_qexpr(void) {
    LVAL* v = malloc(sizeof(LVAL));
    v->type = LVAL_QEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

LVAL* lval_fun(LBUILTIN func, char *name) {
    LVAL* v = malloc(sizeof(LVAL));
    v->type = LVAL_FUN;
    v->builtin = func;
    v->sym = malloc(strlen(name) + 1);
    strcpy(v->sym, name);
    return v;
}

LVAL* lval_lambda(LVAL* formals, LVAL* body) {
    LVAL* v = malloc(sizeof(LVAL));
    v->type = LVAL_FUN;
    v->builtin = NULL;
    v->formals = formals;
    v->body = body;

    /* Build new environment */
    v->env = lenv_new();

    return v;
}

LVAL* lval_err(char* fmt, ...) {
    LVAL* v = malloc(sizeof(LVAL));
    v->type = LVAL_ERR;
    v->err = malloc(512);

    va_list va;
    va_start(va, fmt);

    /* printf the error string with a maximum of 511 characters */
    vsnprintf(v->err, 511, fmt, va);

    v->err = realloc(v->err, strlen(v->err) + 1);
    va_end(va);

    return v;
}

/* LVAL utils */

int lval_eq(LVAL* x, LVAL* y) {
    if (x->type != y->type) { return 0; }

    switch (x->type) {
    case LVAL_NUM: return (x->num == y->num);
    case LVAL_ERR: return (strcmp(x->err, y->err) == 0);
    case LVAL_SYM: return (strcmp(x->sym, y->sym) == 0);
    case LVAL_STR: return (strcmp(x->str, y->str) == 0);

    case LVAL_FUN:
        if (x->builtin || y->builtin) {
            return x->builtin == y->builtin;
        } else {
            return lval_eq(x->formals, y->formals)
                && lval_eq(x->body, y->body);
        }

    case LVAL_QEXPR:
    case LVAL_SEXPR:
        if (x->count != y->count) { return 0; }
        for (int i = 0; i < x->count; i++) {
            if (!lval_eq(x->cell[i], y->cell[i])) { return 0; }
        }
        return 1;
        break;
    }
    return 0;
}

LVAL* lval_add(LVAL* v, LVAL* x) {
    v->count++;
    v->cell = realloc(v->cell, sizeof(LVAL*) * v->count);
    v->cell[v->count-1] = x;
    return v;
}

LVAL* lval_copy(LVAL* v) {
    LVAL* x = malloc(sizeof(LVAL));
    x->type = v->type;

    switch (v->type) {

    /* Copy numbers directly */
    case LVAL_NUM: x->num = v->num; break;

    /* Copy strings using malloc and strcpy */
    case LVAL_ERR:
        x->err = malloc(strlen(v->err) + 1);
        strcpy(x->err, v->err); break;

    case LVAL_SYM:
        x->sym = malloc(strlen(v->sym) + 1);
        strcpy(x->sym, v->sym); break;

    case LVAL_STR:
        x->str = malloc(strlen(v->str) + 1);
        strcpy(x->str, v->str); break;

    case LVAL_FUN:
        x->builtin = v->builtin;

        if (v->builtin) {
            x->sym = malloc(strlen(v->sym) + 1);
            strcpy(x->sym, v->sym);
        }
        else {
            x->env = lenv_copy(v->env);
            x->formals = lval_copy(v->formals);
            x->body = lval_copy(v->body);
        }
        break;

    /* Copy lists by copying each sub-expression */
    case LVAL_SEXPR:
    case LVAL_QEXPR:
        x->count = v->count;
        x->cell = malloc(sizeof(LVAL*) * x->count);
        for (int i = 0; i < x->count; i++) {
            x->cell[i] = lval_copy(v->cell[i]);
        }
        break;
    }

    return x;
}

void lval_del(LVAL* v) {

    switch (v->type) {
    case LVAL_NUM: break;

    case LVAL_ERR: free(v->err); break;
    case LVAL_SYM: free(v->sym); break;
    case LVAL_STR: free(v->str); break;

    case LVAL_FUN:
        if (!v->builtin) {
            lenv_del(v->env);
            lval_del(v->formals);
            lval_del(v->body);
        }
        break;

    /* If Sexpr or Qexpr, delete all the nested elements */
    case LVAL_SEXPR:
    case LVAL_QEXPR:
        for (int i = 0; i < v->count; i++) {
            lval_del(v->cell[i]);
        }
        /* Also free the memory allocated to contain the pointers */
        free(v->cell);
        break;
    }

    /* Free the memory allocated for the "LVAL" struct itself */
    free(v);
}

/* Extract an i-th element from an sexpr */
LVAL* lval_pop(LVAL* v, int i) {
    LVAL* x = v->cell[i];

    /* Shift memory after the item at "i" over the top */
    memmove(&v->cell[i], &v->cell[i+1],
            sizeof(LVAL*) * (v->count-i-1));

    /* Decrease the count of items in the list */
    v->count--;

    /* Reallocate the memory used */
    v->cell = realloc(v->cell, sizeof(LVAL*) * v->count);
    return x;
}

/* Extract an i-th element from an sexpr and delete the rest */
LVAL* lval_take(LVAL* v, int i) {
    LVAL* x = lval_pop(v, i);
    lval_del(v);
    return x;
}

LVAL* lval_join(LVAL* x, LVAL* y) {
    while (y->count) {
        x = lval_add(x, lval_pop(y, 0));
    }
    lval_del(y);
    return x;
}

void lval_print_str(LVAL* v) {
    char* escaped = malloc(strlen(v->str) + 1);
    strcpy(escaped, v->str);
    escaped = mpcf_escape(escaped);
    printf("\"%s\"", escaped);
    free(escaped);
}

void lval_print_expr(LVAL* v, char open, char close) {
    putchar(open);
    for (int i = 0; i < v->count; i++) {
        lval_print(v->cell[i]);

        /* Don't print trailing space if last element */
        if (i != (v->count-1)) {
            putchar(' ');
        }
    }
    putchar(close);
}

void lval_print(LVAL* v) {
    switch (v->type) {
    case LVAL_NUM:   printf("%li", v->num); break;
    case LVAL_ERR:   printf("Error: %s", v->err); break;
    case LVAL_SYM:   printf("%s", v->sym); break;
    case LVAL_STR:   lval_print_str(v); break;
    case LVAL_SEXPR: lval_print_expr(v, '(', ')'); break;
    case LVAL_QEXPR: lval_print_expr(v, '{', '}'); break;
    case LVAL_FUN:
        if (v->builtin) {
            printf("<%s>", v->sym);
        } else {
            printf("(-> "); lval_print(v->formals);
            putchar(' '); lval_print(v->body); putchar(')');
        }
    }
}

void lval_println(LVAL* v) {
    lval_print(v); putchar('\n');
}

LVAL* lval_read_num(mpc_ast_t* t) {
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    if (errno != ERANGE) {
        return lval_num(x);
    }
    else {
        return lval_err("invalid number");
    }
}

LVAL* lval_read_str(mpc_ast_t* t) {
    /* Cut off the final quote character */
    t->contents[strlen(t->contents) - 1] = '\0';

    /* Copy the string missing out the first quote character */
    char* unescaped = malloc(strlen(t->contents + 1) + 1);
    strcpy(unescaped, t->contents + 1);

    /* Pass through the unescape function */
    unescaped = mpcf_unescape(unescaped);

    /* Construct a new LVAL using the string */
    LVAL* str = lval_str(unescaped);

    /* Free the string and return */
    free(unescaped);
    return str;
}

LVAL* lval_read(mpc_ast_t* t) {
    /* If Symbol or Number return conversion to that type */
    if (strstr(t->tag, "number")) { return lval_read_num(t); }
    if (strstr(t->tag, "string")) { return lval_read_str(t); }
    if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }

    /* If root (>) or expr, create an empty list */
    LVAL* x = NULL;
    if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
    if (strstr(t->tag, "sexpr"))  { x = lval_sexpr(); }
    if (strstr(t->tag, "qexpr"))  { x = lval_qexpr(); }

    /* Fill this list with any valid expression contained within */
    for (int i = 0; i < t->children_num; i++) {
        if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
        if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
        if (strcmp(t->children[i]->contents, "}") == 0) { continue; }
        if (strcmp(t->children[i]->contents, "{") == 0) { continue; }
        if (strcmp(t->children[i]->tag,  "regex") == 0) { continue; }
        if (strstr(t->children[i]->tag, "comment")) { continue; }
        x = lval_add(x, lval_read(t->children[i]));
    }

    return x;
}

LVAL* lval_call(LENV* e, LVAL* f, LVAL* a) {

    /* If Builtin then simply apply that */
    if (f->builtin) { return f->builtin(e, a); }

    /* Record argument counts */
    int given = a->count;
    int total = f->formals->count;

    /* While arguments still remain to be processed */
    while (a->count) {

        /* If we've ran out of formal arguments to bind */
        if (f->formals->count == 0) {
            lval_del(a);
            return lval_err("Function passed too many arguments. "
                            "Got %i, expected %i.", given, total);
        }

        /* Pop the first symbol from the formals */
        LVAL* sym = lval_pop(f->formals, 0);

        /* Special case to deal with '&' */
        if (strcmp(sym->sym, "&") == 0) {

            /* Ensure '&' is followed by another symbol */
            if (f->formals->count != 1) {
                lval_del(a);
                return lval_err("Function format invalid. "
                                "Symbol '&' not followed by single symbol.");
            }

            /* Next formal should be bound to remaining arguments */
            LVAL* nsym = lval_pop(f->formals, 0);
            lenv_put(f->env, nsym, builtin_list(e, a));
            lval_del(sym);
            lval_del(nsym);
            break;
        }

        /* Pop the next argument from the list */
        LVAL* val = lval_pop(a, 0);

        /* Bind a copy into the function's environment */
        lenv_put(f->env, sym, val);

        /* Delete symbol and value */
        lval_del(sym); lval_del(val);
    }

    /* If '&' remains in formal list, bind to empty list */
    if (f->formals->count > 0 &&
        strcmp(f->formals->cell[0]->sym, "&") == 0) {

        /* Check to ensure that & is not passed invalidly. */
        if (f->formals->count != 2) {
            return lval_err("Function format invalid. "
                            "Symbol '&' not followed by single symbol.");
        }

        /* Pop and delete '&' symbol */
        lval_del(lval_pop(f->formals, 0));

        /* Pop next symbol and create empty list */
        LVAL* sym = lval_pop(f->formals, 0);
        LVAL* val = lval_qexpr();

        /* Bind to environment and delete */
        lenv_put(f->env, sym, val);
        lval_del(sym); lval_del(val);
    }

    /* Argument list is now bound so can be cleaned up */
    lval_del(a);

    /* If all formals have been bound, evaluate */
    if (f->formals->count == 0) {

        /* Set environment parent to evaluation environment */
        f->env->parent = e;

        /* Evaluate and return */
        return builtin_eval(f->env, lval_add(lval_sexpr(), lval_copy(f->body)));
    }
    /* Otherwise return partially evaluated function */
    else {
        return lval_copy(f);
    }
}

LVAL* lval_eval_sexpr(LENV* e, LVAL* v) {
    /* Eval children */
    for (int i = 0; i < v->count; i++) {
        v->cell[i] = lval_eval(e, v->cell[i]);
    }

    /* Error checking */
    for (int i = 0; i < v->count; i++) {
        if (v->cell[i]->type == LVAL_ERR) { return lval_take(v, i); }
    }

    /* Empty and single expressions */
    if (v->count == 0) { return v; }
    if (v->count == 1) { return lval_take(v, 0); }

    /* Ensure first element is a function after evaluation */
    LVAL* f = lval_pop(v, 0);
    if (f->type != LVAL_FUN) {
        LVAL* err = lval_err(
            "S-Expression starts with incorrect type. "
            "Got %s, expected %s.",
            ltype_name(f->type), ltype_name(LVAL_FUN));
        lval_del(f);
        lval_del(v);
        return err;
    }

    /* Call function and return the result */
    LVAL* result = lval_call(e, f, v);
    lval_del(f);
    return result;
}

LVAL* lval_eval(LENV* e, LVAL* v) {
    if (v->type == LVAL_SYM) {
        LVAL* x = lenv_get(e, v);
        lval_del(v);
        return x;
    }
    if (v->type == LVAL_SEXPR) { return lval_eval_sexpr(e, v); }
    return v;
}
