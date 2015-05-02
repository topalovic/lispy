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

/* Enumeration of LVAL types */
enum { LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR };

/* Enumeration of error types */
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

/* LVAL type */
typedef struct LVAL {
    int type;
    long num;
    char* err;
    char* sym;
    int count;
    struct LVAL** cell;
} LVAL;

/* Number constructor */
LVAL* lval_num(long x) {
    LVAL* v = malloc(sizeof(LVAL));
    v->type = LVAL_NUM;
    v->num = x;
    return v;
}

/* Error constructor */
LVAL* lval_err(char* m) {
    LVAL* v = malloc(sizeof(LVAL));
    v->type = LVAL_ERR;
    v->err = malloc(strlen(m) + 1);
    strcpy(v->err, m);
    return v;
}

/* Symbol constructor */
LVAL* lval_sym(char* s) {
    LVAL* v = malloc(sizeof(LVAL));
    v->type = LVAL_SYM;
    v->sym = malloc(strlen(s) + 1);
    strcpy(v->sym, s);
    return v;
}

/* Sexpr constructor */
LVAL* lval_sexpr(void) {
    LVAL* v = malloc(sizeof(LVAL));
    v->type = LVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

/* A pointer to a new empty Qexpr lval */
LVAL* lval_qexpr(void) {
    LVAL* v = malloc(sizeof(LVAL));
    v->type = LVAL_QEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

LVAL* lval_add(LVAL* v, LVAL* x) {
    v->count++;
    v->cell = realloc(v->cell, sizeof(LVAL*) * v->count);
    v->cell[v->count-1] = x;
    return v;
}

void lval_del(LVAL* v) {

    switch (v->type) {
    case LVAL_NUM: break;

        /* For Err or Sym free the string data */
    case LVAL_ERR: free(v->err); break;
    case LVAL_SYM: free(v->sym); break;

        /* If Sexpr or Qexpr, delete all nested elements */
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
    }
}

void lval_println(LVAL* v) { lval_print(v); putchar('\n'); }

LVAL* lval_eval(LVAL* v);

#define LASSERT(args, cond, err) \
    if (!(cond)) { lval_del(args); return lval_err(err); }

LVAL* builtin_list(LVAL* a) {
    a->type = LVAL_QEXPR;
    return a;
}

LVAL* builtin_len(LVAL* a) {
    LVAL *x = lval_num(a->count);
    lval_del(a);
    return x;
}

LVAL* builtin_head(LVAL* a) {
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

LVAL* builtin_tail(LVAL* a) {
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

LVAL* builtin_eval(LVAL* a) {
    LASSERT(a, a->count == 1,
            "Function 'eval' passed too many arguments!");
    LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
            "Function 'eval' passed incorrect type!");

    LVAL* x = lval_take(a, 0);
    x->type = LVAL_SEXPR;
    return lval_eval(x);
}

LVAL* builtin_join(LVAL* a) {
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

LVAL* builtin_cons(LVAL *a) {
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

LVAL* builtin(LVAL* a, char* func) {
    if (strcmp("list", func) == 0) { return builtin_list(a); }
    if (strcmp("len",  func) == 0) { return builtin_len(a);  }
    if (strcmp("head", func) == 0) { return builtin_head(a); }
    if (strcmp("tail", func) == 0) { return builtin_tail(a); }
    if (strcmp("join", func) == 0) { return builtin_join(a); }
    if (strcmp("cons", func) == 0) { return builtin_cons(a); }
    if (strcmp("eval", func) == 0) { return builtin_eval(a); }
    if (strstr("+-*/%", func)) { return builtin_op(a, func); }
    lval_del(a);
    return lval_err("Unknown Function!");
}

LVAL* lval_eval_sexpr(LVAL* v) {

    /* Eval children */
    for (int i = 0; i < v->count; i++) {
        v->cell[i] = lval_eval(v->cell[i]);
    }

    /* Error checking */
    for (int i = 0; i < v->count; i++) {
        if (v->cell[i]->type == LVAL_ERR) { return lval_take(v, i); }
    }

    /* Empty and single expressions */
    if (v->count == 0) { return v; }
    if (v->count == 1) { return lval_take(v, 0); }

    /* Ensure first element is a Symbol */
    LVAL* f = lval_pop(v, 0);
    if (f->type != LVAL_SYM) {
        lval_del(f); lval_del(v);
        return lval_err("S-expression doesn't start with symbol");
    }

    /* Call builtin */
    LVAL* result = builtin(v, f->sym);
    lval_del(f);
    return result;
}

LVAL* lval_eval(LVAL* v) {
    /* Evaluate sexprs */
    if (v->type == LVAL_SEXPR) { return lval_eval_sexpr(v); }

    /* All other LVAL types remain the same */
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
    "                                                              \
      number : /-?[0-9]+/ ;                                        \
      symbol : \"list\" | \"len\" | \"head\" | \"tail\" | \"eval\" \
             | \"join\" | \"cons\" | '+' | '-' | '*' | '/' ;       \
      sexpr  : '(' <expr>* ')' ;                                   \
      qexpr  : '{' <expr>* '}' ;                                   \
      expr   : <number> | <symbol> | <sexpr> | <qexpr> ;           \
      lispy  : /^/ <expr>* /$/ ;                                   \
    ",
    Number, Symbol, Sexpr ,Qexpr, Expr, Lispy);

    puts("Lispy Version 0.0.3");
    puts("Press Ctrl+C to Exit\n");

    while (1) {
        char* input = readline("lispy> ");
        if (!input) break;
        add_history(input);

        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lispy, &r)) {
            LVAL* x = lval_eval(lval_read(r.output));
            printf("=> ");
            lval_println(x);
            lval_del(x);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }
        free(input);
    }

    mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Lispy);

    return 0;
}
