#include "lispy.h"

/* Lispy! */

/**
 * Load and parse file within a context of a given lenv.
 */
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

/**
 * Register builtins for a given lenv.
 */
void register_builtins(LENV* e) {

    /* String functions */
    lenv_register_builtin(e, "load",  builtin_load);
    lenv_register_builtin(e, "print", builtin_print);
    lenv_register_builtin(e, "error", builtin_error);

    /* Var functions */
    lenv_register_builtin(e, "def",  builtin_def);
    lenv_register_builtin(e, "=",    builtin_put);
    lenv_register_builtin(e, "->",   builtin_lambda);
    lenv_register_builtin(e, "type", builtin_type);

    /* List functions */
    lenv_register_builtin(e, "list", builtin_list);
    lenv_register_builtin(e, "len",  builtin_len);
    lenv_register_builtin(e, "head", builtin_head);
    lenv_register_builtin(e, "tail", builtin_tail);
    lenv_register_builtin(e, "eval", builtin_eval);
    lenv_register_builtin(e, "join", builtin_join);
    lenv_register_builtin(e, "cons", builtin_cons);

    /* Math functions */
    lenv_register_builtin(e, "+", builtin_add);
    lenv_register_builtin(e, "-", builtin_sub);
    lenv_register_builtin(e, "*", builtin_mul);
    lenv_register_builtin(e, "/", builtin_div);
    lenv_register_builtin(e, "%", builtin_mod);

    /* Comparison functions */
    lenv_register_builtin(e, "if",  builtin_if);
    lenv_register_builtin(e, "==",  builtin_eq);
    lenv_register_builtin(e, "!=",  builtin_ne);
    lenv_register_builtin(e, ">",   builtin_gt);
    lenv_register_builtin(e, "<",   builtin_lt);
    lenv_register_builtin(e, ">=",  builtin_ge);
    lenv_register_builtin(e, "<=",  builtin_le);
}

/**
 * Load lib within a given lenv.
 */
void load_lib(LENV* e, char *filename) {
    LVAL* arg = lval_add(lval_sexpr(), lval_str(filename));
    LVAL* res = builtin_load(e, arg);

    if (res->type == LVAL_ERR) { lval_println(res); }
    lval_del(res);
}

/**
 * Start the interpreter.
 */
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
    register_builtins(e);

    load_lib(e, "prologue.lsp");

    char *prompt = ">> ";
    char *result = "=> ";

    while (1) {
        char* input = readline(prompt);
        if (!input) {
            puts("\nTa-ta.");
            break;
        }
        add_history(input);

        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lispy, &r)) {
            LVAL* x = lval_eval(e, lval_read(r.output));

            printf("%s", result);
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
