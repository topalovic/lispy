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

/* Enumeration of lval types */
enum { LVAL_NUM, LVAL_ERR };

/* Enumeration of error types */
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

/* LVAL type */
typedef struct {
    int type;
    long num;
    int err;
} LVAL;

/* Return a new number LVAL */
LVAL lval_num(long x) {
    LVAL v;
    v.type = LVAL_NUM;
    v.num = x;
    return v;
}

/* Return a new error LVAL */
LVAL lval_err(int x) {
    LVAL v;
    v.type = LVAL_ERR;
    v.err = x;
    return v;
}

/* Print LVAL */
void lval_print(LVAL v) {
    switch (v.type) {
    case LVAL_NUM:
        printf("%li", v.num);
        break;
    case LVAL_ERR:
        switch (v.err) {
        case LERR_DIV_ZERO:
            printf("Error: Division by zero!");
            break;
        case LERR_BAD_OP:
            printf("Error: Invalid operator!");
            break;
        case LERR_BAD_NUM:
            printf("Error: Invalid number!");
            break;
        }
    }
}

/* Print LVAL followed by a newline */
void lval_println(LVAL v) {
    lval_print(v);
    putchar('\n');
}

LVAL eval_op(LVAL x, char* op, LVAL y) {
    /* If either value is an error return it */
    if (x.type == LVAL_ERR) { return x; }
    if (y.type == LVAL_ERR) { return y; }

    /* Otherwise do maths on the number values */
    if (strcmp(op, "+") == 0) { return lval_num(x.num + y.num); }
    if (strcmp(op, "-") == 0) { return lval_num(x.num - y.num); }
    if (strcmp(op, "*") == 0) { return lval_num(x.num * y.num); }
    if (strcmp(op, "/") == 0) {
        /* If second operand is zero return error */
        return y.num == 0
            ? lval_err(LERR_DIV_ZERO)
            : lval_num(x.num / y.num);
    }
    return lval_err(LERR_BAD_OP);
}

LVAL eval(mpc_ast_t* t) {
    if (strstr(t->tag, "number")) {
        /* Check for conversion errors */
        errno = 0;
        long x = strtol(t->contents, NULL, 10);
        return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
    }

    char* op = t->children[1]->contents;
    LVAL x = eval(t->children[2]);

    int i = 3;
    while (strstr(t->children[i]->tag, "expr")) {
        x = eval_op(x, op, eval(t->children[i]));
        i++;
    }

    return x;
}

int main(int argc, char** argv) {
    mpc_parser_t* Number   = mpc_new("number");
    mpc_parser_t* Operator = mpc_new("operator");
    mpc_parser_t* Expr     = mpc_new("expr");
    mpc_parser_t* Lispy    = mpc_new("lispy");

    mpca_lang(MPCA_LANG_DEFAULT,
              "                                                     \
    number   : /-?[0-9]+/ ;                                         \
    operator : '+' | '-' | '*' | '/' ;                              \
    expr     : <number> | '(' <operator> <expr>+ ')' ;              \
    lispy    : /^/ <operator> <expr>+ /$/ ;                         \
  ",
              Number, Operator, Expr, Lispy);

    puts("Lispy Version 0.0.1");
    puts("Press Ctrl+C to Exit\n");

    while (1) {
        char* input = readline("lispy> ");
        add_history(input);

        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lispy, &r)) {
            LVAL result = eval(r.output);
            lval_println(result);
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }
        free(input);
    }

    mpc_cleanup(4, Number, Operator, Expr, Lispy);

    return 0;
}
