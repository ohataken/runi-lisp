#ifndef RUNI_LISP_H
#define RUNI_LISP_H
#define RUNI_SYMBOL_MAX_LEN 200

#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

enum {
    RUNI_INTEGER = 1,
    RUNI_LIST,
    RUNI_SYMBOL,
    RUNI_PRIMITIVE,
    RUNI_FUNCTION,
    RUNI_MACRO,
    RUNI_ENV,
    RUNI_NIL,
    RUNI_DOT,
    RUNI_CPAREN,
    RUNI_TRUE,
};

struct runi_object;

typedef struct runi_object *runi_primitive(struct runi_object *env, struct runi_object *args);

struct runi_object {
    int type;

    union {
        int integer;

        struct {
            struct runi_object *car;
            struct runi_object *cdr;
        };

        char name[1];

        runi_primitive *fn;

        struct {
            struct runi_object *env;
            struct runi_object *args;
            struct runi_object *body;
        };

        struct {
            struct runi_object *vars;
            struct runi_object *parent;
        };
    };
};

extern struct runi_object *runi_nil;
extern struct runi_object *runi_dot;
extern struct runi_object *runi_cparen;
extern struct runi_object *runi_true;
extern struct runi_object *runi_symbols;

void __attribute((noreturn)) runi_error(char *fmt, ...);

struct runi_object *runi_make_integer(int integer);

struct runi_object *runi_make_symbol(char *name);

struct runi_object *runi_make_primitive(runi_primitive *fn);

struct runi_object *runi_make_function(int type, struct runi_object *env, struct runi_object *args, struct runi_object *body);

struct runi_object *runi_make_special(int type);

struct runi_object *runi_make_env(struct runi_object *vars, struct runi_object *parent);

struct runi_object *runi_cons(struct runi_object *car, struct runi_object *cdr);

struct runi_object *runi_acons(struct runi_object *x, struct runi_object *y, struct runi_object *a);

struct runi_object *runi_parse(void);

struct runi_object *runi_intern(char *name);

void runi_print(struct runi_object *obj);

void runi_add_variable(struct runi_object *env, struct runi_object *sym, struct runi_object *val);

struct runi_object *runi_eval_list(struct runi_object *env, struct runi_object *list);

struct runi_object *runi_find(struct runi_object *env, struct runi_object *sym);

bool runi_is_list(struct runi_object *obj);

int runi_list_length(struct runi_object *list);

struct runi_object *runi_macroexpand(struct runi_object *env, struct runi_object *obj);

struct runi_object *runi_progn(struct runi_object *env, struct runi_object *list);

struct runi_object *runi_eval(struct runi_object *env, struct runi_object *obj);

#endif
