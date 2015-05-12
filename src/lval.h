#ifndef lval_h
#define lval_h

#include "mpc.h"
#include "lenv.h"

enum {
    LVAL_ERR,
    LVAL_NUM,
    LVAL_SYM,
    LVAL_STR,
    LVAL_SEXPR,
    LVAL_QEXPR,
    LVAL_FUN
};

struct LENV;
struct LVAL;
typedef struct LVAL LVAL;

typedef LVAL*(*LBUILTIN)(struct LENV*, LVAL*);

struct LVAL {
    int type;

    /* Basic */
    long num;
    char* err;
    char* sym;
    char* str;

    /* Function */
    LBUILTIN builtin;
    struct LENV* env;
    LVAL* formals;
    LVAL* body;

    /* Expression */
    int count;
    LVAL** cell;
};

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

char* ltype_name(int t);

LVAL* lval_num(long x);
LVAL* lval_sym(char* s);
LVAL* lval_str(char* s);
LVAL* lval_sexpr(void);
LVAL* lval_qexpr(void);
LVAL* lval_fun(LBUILTIN func, char *name);
LVAL* lval_lambda(LVAL* formals, LVAL* body);
LVAL* lval_err(char* fmt, ...);

int   lval_eq(LVAL* x, LVAL* y);
LVAL* lval_add(LVAL* v, LVAL* x);
LVAL* lval_copy(LVAL* v);
void  lval_del(LVAL* v);
LVAL* lval_pop(LVAL* v, int i);
LVAL* lval_take(LVAL* v, int i);
LVAL* lval_join(LVAL* x, LVAL* y);

void lval_print_str(LVAL* v);
void lval_print_expr(LVAL* v, char open, char close);
void lval_print(LVAL* v);
void lval_println(LVAL* v);

LVAL* lval_read_num(mpc_ast_t* t);
LVAL* lval_read_str(mpc_ast_t* t);
LVAL* lval_read(mpc_ast_t* t);

LVAL* lval_call(struct LENV* e, LVAL* f, LVAL* a);
LVAL* lval_eval_sexpr(struct LENV* e, LVAL* v);
LVAL* lval_eval(struct LENV* e, LVAL* v);

LVAL* builtin_eval(struct LENV *e, LVAL* a);
LVAL* builtin_list(struct LENV *e, LVAL* a);

#endif
