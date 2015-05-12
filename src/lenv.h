#ifndef lenv_h
#define lenv_h

#include "lval.h"

struct LENV;
struct LVAL;
typedef struct LENV LENV;

struct LENV {
    LENV* parent;
    int count;
    char** syms;
    struct LVAL** vals;
};

LENV* lenv_new(void);
LENV* lenv_copy(LENV* e);
void  lenv_del(LENV* e);

struct LVAL* lenv_get(LENV* e, struct LVAL* k);
void lenv_put(LENV* e, struct LVAL* k, struct LVAL* v);

void lenv_def(LENV* e, struct LVAL* k, struct LVAL* v);

#endif
