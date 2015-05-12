#include "lenv.h"
#include "lval.h"

/* Lisp environments (scopes) */

/* Env constructor */
LENV* lenv_new(void) {
    LENV* e = malloc(sizeof(LENV));
    e->parent = NULL;
    e->count = 0;
    e->syms = NULL;
    e->vals = NULL;
    return e;
}

/* Copy constructor */
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

    /* If no symbol found, check the ancestor tree */
    if (e->parent) {
        return lenv_get(e->parent, k);
    } else {
        return lval_err("Unbound symbol '%s'", k->sym);
    }
}

void lenv_put(LENV* e, LVAL* k, LVAL* v) {
    for (int i = 0; i < e->count; i++) {
        /* If variable found, delete item at that position */
        /* and replace with variable supplied by user */
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

/* Global "put" */
void lenv_def(LENV* e, LVAL* k, LVAL* v) {
    /* Get global env */
    while (e->parent) { e = e->parent; }
    lenv_put(e, k, v);
}
