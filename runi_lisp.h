#ifndef RUNI_LISP_H
#define RUNI_LISP_H

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

#endif
