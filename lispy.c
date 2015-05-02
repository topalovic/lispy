#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"

/* No history.h on OS X */
#ifdef __APPLE__
#include <editline/readline.h>
#else
#include <editline/readline.h>
#include <editline/history.h>
#endif

struct LVAL;
struct LENV;
typedef struct LVAL LVAL;
typedef struct LENV LENV;

/* Lisp Value */

enum {
    LVAL_ERR,
    LVAL_NUM,
    LVAL_SYM,
    LVAL_SEXPR,
    LVAL_QEXPR,
    LVAL_FUN
};

typedef LVAL*(*LBUILTIN)(LENV*, LVAL*);

struct LVAL {
    int type;
    long num;
    char* err;
    char* sym;
    int count;
    LBUILTIN fun;
    struct LVAL** cell;
};

/* LVAL constructors */

LVAL* lval_err(char* m) {
    LVAL* v = malloc(sizeof(LVAL));
    v->type = LVAL_ERR;
    v->err = malloc(strlen(m) + 1);
    strcpy(v->err, m);
    return v;
}

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

LVAL* lval_fun(LBUILTIN func) {
  LVAL* v = malloc(sizeof(LVAL));
  v->type = LVAL_FUN;
  v->fun = func;
  return v;
}

/* LVAL utils */

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

    /* Copy functions and numbers directly */
    case LVAL_FUN: x->fun = v->fun; break;
    case LVAL_NUM: x->num = v->num; break;

    /* Copy strings using malloc and strcpy */
    case LVAL_ERR:
        x->err = malloc(strlen(v->err) + 1);
        strcpy(x->err, v->err); break;

    case LVAL_SYM:
        x->sym = malloc(strlen(v->sym) + 1);
        strcpy(x->sym, v->sym); break;

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

    /* For Err or Sym free the string data */
    case LVAL_ERR: free(v->err); break;
    case LVAL_SYM: free(v->sym); break;
    case LVAL_FUN: break;

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

LVAL* lval_read(mpc_ast_t* t) {

    /* If Symbol or Number return conversion to that type */
    if (strstr(t->tag, "number")) { return lval_read_num(t); }
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

void lval_print(LVAL* v) {
    switch (v->type) {
    case LVAL_NUM:   printf("%li", v->num); break;
    case LVAL_ERR:   printf("Error: %s", v->err); break;
    case LVAL_SYM:   printf("%s", v->sym); break;
    case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
    case LVAL_QEXPR: lval_expr_print(v, '{', '}'); break;
    case LVAL_FUN:   printf("<function>"); break;
    }
}

void lval_println(LVAL* v) { lval_print(v); putchar('\n'); }

/* Lisp Environment */

struct LENV {
  int count;
  char** syms;
  LVAL** vals;
};


/* Env constructor */
LENV* lenv_new(void) {
    LENV* e = malloc(sizeof(LENV));
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

LVAL* lenv_get(LENV* e, LVAL* k) {
    for (int i = 0; i < e->count; i++) {
        if (strcmp(e->syms[i], k->sym) == 0) {
            return lval_copy(e->vals[i]);
        }
    }
    /* If no symbol found return error */
    return lval_err("unbound symbol!");
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

/* Builtins */

#define LASSERT(args, cond, err) \
    if (!(cond)) { lval_del(args); return lval_err(err); }

LVAL* lval_eval(LENV* e, LVAL* v);

LVAL* builtin_def(LENV* e, LVAL* a) {
    LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
            "Function 'def' passed incorrect type!");

    /* First argument is symbol list */
    LVAL* syms = a->cell[0];

    /* Ensure all elements of first list are symbols */
    for (int i = 0; i < syms->count; i++) {
        LASSERT(a, syms->cell[i]->type == LVAL_SYM,
                "Function 'def' cannot define non-symbol");
    }

    /* Check correct number of symbols and values */
    LASSERT(a, syms->count == a->count - 1,
            "Function 'def' cannot define incorrect "
            "number of values to symbols");

    /* Assign copies of values to symbols */
    for (int i = 0; i < syms->count; i++) {
        lenv_put(e, syms->cell[i], a->cell[i + 1]);
    }

    lval_del(a);
    return lval_sexpr();
}

LVAL* builtin_list(LENV *e, LVAL* a) {
    a->type = LVAL_QEXPR;
    return a;
}

LVAL* builtin_len(LENV *e, LVAL* a) {
    LVAL *x = lval_num(a->count);
    lval_del(a);
    return x;
}

LVAL* builtin_head(LENV *e, LVAL* a) {
    LASSERT(a, a->count == 1,
            "Function 'head' passed too many arguments!");
    LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
            "Function 'head' passed incorrect type!");
    LASSERT(a, a->cell[0]->count != 0,
            "Function 'head' passed {}!");

    LVAL* v = lval_take(a, 0);
    while (v->count > 1) { lval_del(lval_pop(v, 1)); }
    return v;
}

LVAL* builtin_tail(LENV *e, LVAL* a) {
    LASSERT(a, a->count == 1,
            "Function 'tail' passed too many arguments!");
    LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
            "Function 'tail' passed incorrect type!");
    LASSERT(a, a->cell[0]->count != 0,
            "Function 'tail' passed {}!");

    LVAL* v = lval_take(a, 0);
    lval_del(lval_pop(v, 0));
    return v;
}

LVAL* builtin_eval(LENV *e, LVAL* a) {
    LASSERT(a, a->count == 1,
            "Function 'eval' passed too many arguments!");
    LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
            "Function 'eval' passed incorrect type!");

    LVAL* x = lval_take(a, 0);
    x->type = LVAL_SEXPR;
    return lval_eval(e, x);
}

LVAL* builtin_join(LENV *e, LVAL* a) {
    for (int i = 0; i < a->count; i++) {
        LASSERT(a, a->cell[i]->type == LVAL_QEXPR,
                "Function 'join' passed incorrect type.");
    }

    LVAL* x = lval_pop(a, 0);

    while (a->count) {
        x = lval_join(x, lval_pop(a, 0));
    }

    lval_del(a);
    return x;
}

LVAL* builtin_cons(LENV *e, LVAL *a) {
    LASSERT(a, a->count == 2,
            "Function 'cons' passed too many arguments!");
    LASSERT(a, a->cell[1]->type == LVAL_QEXPR,
            "Function 'cons' passed incorrect type!");

    LVAL* x = lval_qexpr();

    x = lval_add(x, lval_pop(a, 0));
    x = lval_join(x, lval_pop(a, 0));

    lval_del(a);
    return x;
}

LVAL* builtin_op(LVAL* a, char* op) {

    /* Ensure all arguments are numbers */
    for (int i = 0; i < a->count; i++) {
        if (a->cell[i]->type != LVAL_NUM) {
            lval_del(a);
            return lval_err("Cannot operate on non-number");
        }
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

void lenv_add_builtin(LENV* e, char* name, LBUILTIN func) {
    LVAL* k = lval_sym(name);
    LVAL* v = lval_fun(func);
    lenv_put(e, k, v);
    lval_del(k);
    lval_del(v);
}

void lenv_add_builtins(LENV* e) {
    /* Var Functions */
    lenv_add_builtin(e, "def",  builtin_def);

    /* List Functions */
    lenv_add_builtin(e, "list", builtin_list);
    lenv_add_builtin(e, "len",  builtin_len);
    lenv_add_builtin(e, "head", builtin_head);
    lenv_add_builtin(e, "tail", builtin_tail);
    lenv_add_builtin(e, "eval", builtin_eval);
    lenv_add_builtin(e, "join", builtin_join);
    lenv_add_builtin(e, "cons", builtin_cons);

    /* Mathematical Functions */
    lenv_add_builtin(e, "+", builtin_add);
    lenv_add_builtin(e, "-", builtin_sub);
    lenv_add_builtin(e, "*", builtin_mul);
    lenv_add_builtin(e, "/", builtin_div);
    lenv_add_builtin(e, "%", builtin_mod);
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
        lval_del(v); lval_del(f);
        return lval_err("first element is not a function");
    }

    /* Call function and return the result */
    LVAL* result = f->fun(e, v);
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
    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* Symbol = mpc_new("symbol");
    mpc_parser_t* Sexpr  = mpc_new("sexpr");
    mpc_parser_t* Qexpr  = mpc_new("qexpr");
    mpc_parser_t* Expr   = mpc_new("expr");
    mpc_parser_t* Lispy  = mpc_new("lispy");

    mpca_lang(MPCA_LANG_DEFAULT,
      "                                                  \
      number : /-?[0-9]+/ ;                              \
      symbol : /[a-zA-Z0-9_+\\-*%\\/\\\\=<>!&]+/ ;       \
      sexpr  : '(' <expr>* ')' ;                         \
      qexpr  : '{' <expr>* '}' ;                         \
      expr   : <number> | <symbol> | <sexpr> | <qexpr> ; \
      lispy  : /^/ <expr>* /$/ ;                         \
      ",
      Number, Symbol, Sexpr, Qexpr, Expr, Lispy);

    puts("Lispy Version 0.0.4");
    puts("Press Ctrl+C to Exit\n");

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

    mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Lispy);

    return 0;
}
