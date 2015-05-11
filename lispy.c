#include "mpc.h"

/* No history.h on OS X */
#ifdef __APPLE__
#include <editline/readline.h>
#else
#include <editline/readline.h>
#include <editline/history.h>
#endif

mpc_parser_t* Number;
mpc_parser_t* Symbol;
mpc_parser_t* String;
mpc_parser_t* Comment;
mpc_parser_t* Sexpr;
mpc_parser_t* Qexpr;
mpc_parser_t* Expr;
mpc_parser_t* Lispy;

struct LVAL;
struct LENV;
typedef struct LVAL LVAL;
typedef struct LENV LENV;

/* Lisp Value */

enum {
    LVAL_ERR,
    LVAL_NUM,
    LVAL_SYM,
    LVAL_STR,
    LVAL_SEXPR,
    LVAL_QEXPR,
    LVAL_FUN
};

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

typedef LVAL*(*LBUILTIN)(LENV*, LVAL*);

struct LENV {
    LENV* parent;
    int count;
    char** syms;
    LVAL** vals;
};

struct LVAL {
    int type;

    /* Basic */
    long num;
    char* err;
    char* sym;
    char* str;

    /* Function */
    LBUILTIN builtin;
    LENV* env;
    LVAL* formals;
    LVAL* body;

    /* Expression */
    int count;
    LVAL** cell;
};

LENV* lenv_new(void);
LENV* lenv_copy(LENV* e);
void lenv_del(LENV* e);
void lenv_put(LENV* e, LVAL* k, LVAL* v);

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

    /* For each cell in 'y' add it to 'x' */
    while (y->count) {
        x = lval_add(x, lval_pop(y, 0));
    }

    /* Delete the empty 'y' and return 'x' */
    lval_del(y);
    return x;
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

void lval_print(LVAL* v);

void lval_expr_print(LVAL* v, char open, char close) {
    putchar(open);
    for (int i = 0; i < v->count; i++) {

        /* Print Value contained within */
        lval_print(v->cell[i]);

        /* Don't print trailing space if last element */
        if (i != (v->count-1)) {
            putchar(' ');
        }
    }
    putchar(close);
}

void lval_print_str(LVAL* v) {
    char* escaped = malloc(strlen(v->str) + 1);
    strcpy(escaped, v->str);
    escaped = mpcf_escape(escaped);
    printf("\"%s\"", escaped);
    free(escaped);
}

void lval_print(LVAL* v) {
    switch (v->type) {
    case LVAL_NUM:   printf("%li", v->num); break;
    case LVAL_ERR:   printf("Error: %s", v->err); break;
    case LVAL_SYM:   printf("%s", v->sym); break;
    case LVAL_STR:   lval_print_str(v); break;
    case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
    case LVAL_QEXPR: lval_expr_print(v, '{', '}'); break;
    case LVAL_FUN:
        if (v->builtin) {
            printf("<%s>", v->sym);
        } else {
            printf("(-> "); lval_print(v->formals);
            putchar(' '); lval_print(v->body); putchar(')');
        }
    }
}

void lval_println(LVAL* v) { lval_print(v); putchar('\n'); }

LVAL* builtin_eval(LENV *e, LVAL* a);
LVAL* builtin_list(LENV *e, LVAL* a);

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

/* Lisp Environment */

/* Env constructor */
LENV* lenv_new(void) {
    LENV* e = malloc(sizeof(LENV));
    e->parent = NULL;
    e->count = 0;
    e->syms = NULL;
    e->vals = NULL;
    return e;
}

/* Env destructor */
void lenv_del(LENV* e) {
    for (int i = 0; i < e->count; i++) {
        free(e->syms[i]);
        lval_del(e->vals[i]);
    }
    free(e->syms);
    free(e->vals);
    free(e);
}

LENV* lenv_copy(LENV* e) {
    LENV* n = malloc(sizeof(LENV));
    n->parent = e->parent;
    n->count = e->count;
    n->syms = malloc(sizeof(char*) * n->count);
    n->vals = malloc(sizeof(LVAL*) * n->count);
    for (int i = 0; i < e->count; i++) {
        n->syms[i] = malloc(strlen(e->syms[i]) + 1);
        strcpy(n->syms[i], e->syms[i]);
        n->vals[i] = lval_copy(e->vals[i]);
    }
    return n;
}

LVAL* lenv_get(LENV* e, LVAL* k) {
    for (int i = 0; i < e->count; i++) {
        if (strcmp(e->syms[i], k->sym) == 0) {
            return lval_copy(e->vals[i]);
        }
    }

    /* If no symbol found, check the ancestor tree */
    if (e->parent) {
        return lenv_get(e->parent, k);
    } else {
        return lval_err("Unbound symbol '%s'", k->sym);
    }
}

void lenv_put(LENV* e, LVAL* k, LVAL* v) {
    for (int i = 0; i < e->count; i++) {
        /* If variable is found delete item at that position */
        /* And replace with variable supplied by user */
        if (strcmp(e->syms[i], k->sym) == 0) {
            lval_del(e->vals[i]);
            e->vals[i] = lval_copy(v);
            return;
        }
    }

    /* If no existing entry found, allocate space for new entry */
    e->count++;
    e->vals = realloc(e->vals, sizeof(LVAL*) * e->count);
    e->syms = realloc(e->syms, sizeof(char*) * e->count);

    /* Copy contents of LVAL and symbol string into new location */
    e->vals[e->count - 1] = lval_copy(v);
    e->syms[e->count - 1] = malloc(strlen(k->sym) + 1);
    strcpy(e->syms[e->count - 1], k->sym);
}

void lenv_def(LENV* e, LVAL* k, LVAL* v) {
    /* Get global env */
    while (e->parent) { e = e->parent; }
    lenv_put(e, k, v);
}

/* Builtins */

#define LASSERT(args, cond, fmt, ...)                   \
    if (!(cond)) {                                      \
        LVAL* err = lval_err(fmt, ##__VA_ARGS__);       \
        lval_del(args);                                 \
        return err;                                     \
    }

#define LASSERT_TYPE(func, args, index, expect)         \
    LASSERT(args, args->cell[index]->type == expect,    \
        "Function '%s' passed incorrect type for argument %i. Got %s, expected %s.", \
        func, index + 1, ltype_name(args->cell[index]->type), ltype_name(expect))

#define LASSERT_NUM(func, args, num)                    \
    LASSERT(args, args->count == num,                   \
        "Function '%s' passed incorrect number of arguments. Got %i, expected %i.", \
        func, args->count, num)

#define LASSERT_NOT_EMPTY(func, args, index)            \
    LASSERT(args, args->cell[index]->count != 0,        \
    "Function '%s' passed {} for argument %i.", func, index + 1);

LVAL* lval_eval(LENV* e, LVAL* v);

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

LVAL* builtin_eval(LENV *e, LVAL* a) {
    LASSERT_NUM("eval", a, 1);
    LASSERT_TYPE("eval", a, 0, LVAL_QEXPR);

    LVAL* x = lval_take(a, 0);
    x->type = LVAL_SEXPR;
    return lval_eval(e, x);
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

LVAL* builtin_load(LENV* e, LVAL* a) {
    LASSERT_NUM("load", a, 1);
    LASSERT_TYPE("load", a, 0, LVAL_STR);

    /* Parse file given by string name */
    mpc_result_t r;
    if (mpc_parse_contents(a->cell[0]->str, Lispy, &r)) {
        LVAL* expr = lval_read(r.output);
        mpc_ast_delete(r.output);

        while (expr->count) {
            LVAL* x = lval_eval(e, lval_pop(expr, 0));
            if (x->type == LVAL_ERR) { lval_println(x); }
            lval_del(x);
        }

        lval_del(expr);
        lval_del(a);
        return lval_sexpr();
    }
    else {
        char* err_msg = mpc_err_string(r.error);
        mpc_err_delete(r.error);

        LVAL* err = lval_err("Could not load %s", err_msg);
        free(err_msg);
        lval_del(a);
        return err;
    }
}

LVAL* builtin_print(LENV* e, LVAL* a) {
    for (int i = 0; i < a->count; i++) {
        lval_print(a->cell[i]); putchar(' ');
    }

    putchar('\n');
    lval_del(a);

    return lval_sexpr();
}

LVAL* builtin_type(LENV* e, LVAL* a) {
    char *s = ltype_name(a->cell[0]->type);
    lval_del(a);
    return lval_str(s);
}

LVAL* builtin_error(LENV* e, LVAL* a) {
    LASSERT_NUM("error", a, 1);
    LASSERT_TYPE("error", a, 0, LVAL_STR);

    LVAL* err = lval_err(a->cell[0]->str);

    lval_del(a);
    return err;
}

void lenv_add_builtin(LENV* e, char* name, LBUILTIN func) {
    LVAL* k = lval_sym(name);
    LVAL* v = lval_fun(func, name);
    lenv_put(e, k, v);
    lval_del(k);
    lval_del(v);
}

void lenv_add_builtins(LENV* e) {
    lenv_add_builtin(e, "type", builtin_type);

    /* String Functions */
    lenv_add_builtin(e, "load",  builtin_load);
    lenv_add_builtin(e, "error", builtin_error);
    lenv_add_builtin(e, "print", builtin_print);

    /* Var functions */
    lenv_add_builtin(e, "def", builtin_def);
    lenv_add_builtin(e, "=",   builtin_put);
    lenv_add_builtin(e, "->",  builtin_lambda);

    /* List functions */
    lenv_add_builtin(e, "list", builtin_list);
    lenv_add_builtin(e, "len",  builtin_len);
    lenv_add_builtin(e, "head", builtin_head);
    lenv_add_builtin(e, "tail", builtin_tail);
    lenv_add_builtin(e, "eval", builtin_eval);
    lenv_add_builtin(e, "join", builtin_join);
    lenv_add_builtin(e, "cons", builtin_cons);

    /* Math functions */
    lenv_add_builtin(e, "+", builtin_add);
    lenv_add_builtin(e, "-", builtin_sub);
    lenv_add_builtin(e, "*", builtin_mul);
    lenv_add_builtin(e, "/", builtin_div);
    lenv_add_builtin(e, "%", builtin_mod);

    /* Comparison functions*/
    lenv_add_builtin(e, "if",  builtin_if);
    lenv_add_builtin(e, "==",  builtin_eq);
    lenv_add_builtin(e, "!=",  builtin_ne);
    lenv_add_builtin(e, ">",   builtin_gt);
    lenv_add_builtin(e, "<",   builtin_lt);
    lenv_add_builtin(e, ">=",  builtin_ge);
    lenv_add_builtin(e, "<=",  builtin_le);
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

int main(int argc, char** argv) {

    Number  = mpc_new("number");
    Symbol  = mpc_new("symbol");
    String  = mpc_new("string");
    Comment = mpc_new("comment");
    Sexpr   = mpc_new("sexpr");
    Qexpr   = mpc_new("qexpr");
    Expr    = mpc_new("expr");
    Lispy   = mpc_new("lispy");

    mpca_lang(MPCA_LANG_DEFAULT,
      "                                                  \
      number : /-?[0-9]+/ ;                              \
      symbol : /[a-zA-Z0-9_+\\-*%\\/\\\\=<>!\?&]+/ ;     \
      string  : /\"(\\\\.|[^\"])*\"/ ;                   \
      comment : /;[^\\r\\n]*/ ;                          \
      sexpr  : '(' <expr>* ')' ;                         \
      qexpr  : '{' <expr>* '}' ;                         \
      expr   : <number>   | <symbol> | <string>          \
             | <comment>  | <sexpr>  | <qexpr> ;         \
      lispy  : /^/ <expr>* /$/ ;                         \
      ",
      Number, Symbol, String, Comment, Sexpr, Qexpr, Expr, Lispy);

    LENV* e = lenv_new();
    lenv_add_builtins(e);

    while (1) {
        char* input = readline("lispy> ");
        if (!input) {
            puts("\nTa-ta~!");
            break;
        }
        add_history(input);

        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lispy, &r)) {
            LVAL* x = lval_eval(e, lval_read(r.output));

            printf("=> ");
            lval_println(x);

            lval_del(x);
            mpc_ast_delete(r.output);
        }
        else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        free(input);
    }

    lenv_del(e);

    mpc_cleanup(8,
        Number, Symbol, String, Comment,
        Sexpr,  Qexpr,  Expr,   Lispy);

    return 0;
}
