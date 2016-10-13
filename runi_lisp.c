#include "runi_lisp.h"

struct runi_object *runi_nil = NULL;
struct runi_object *runi_dot = NULL;
struct runi_object *runi_cparen = NULL;
struct runi_object *runi_true = NULL;
struct runi_object *runi_symbols = NULL;

void runi_error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(1);
}

static struct runi_object *runi_alloc(int type, size_t size) {
    size += offsetof(struct runi_object, integer);
    struct runi_object *obj = malloc(size);
    obj->type = type;
    return obj;
}

struct runi_object *runi_make_integer(int integer) {
    struct runi_object *r = runi_alloc(RUNI_INTEGER, sizeof(int));
    r->integer = integer;
    return r;
}

struct runi_object *runi_make_symbol(char *name) {
    struct runi_object *sym = runi_alloc(RUNI_SYMBOL, strlen(name) + 1);
    strcpy(sym->name, name);
    return sym;
}

struct runi_object *runi_make_primitive(runi_primitive *fn) {
    struct runi_object *r = runi_alloc(RUNI_PRIMITIVE, sizeof(runi_primitive *));
    r->fn = fn;
    return r;
}

struct runi_object *runi_make_function(int type, struct runi_object *env, struct runi_object *args, struct runi_object *body) {
    assert(type == RUNI_FUNCTION || type == RUNI_MACRO);
    struct runi_object *r = runi_alloc(type, sizeof(struct runi_object *) * 3);
    r->env = env;
    r->args = args;
    r->body = body;
    return r;
}

struct runi_object *runi_make_special(int type) {
    struct runi_object *r = malloc(sizeof(void *) * 2);
    r->type = type;
    return r;
}

struct runi_object *runi_make_env(struct runi_object *vars, struct runi_object *parent) {
    struct runi_object *r = runi_alloc(RUNI_ENV, sizeof(struct runi_object *) * 2);
    r->vars = vars;
    r->parent = parent;
    return r;
}

struct runi_object *runi_cons(struct runi_object *car, struct runi_object *cdr) {
    struct runi_object *cell = runi_alloc(RUNI_LIST, sizeof(struct runi_object *) * 2);
    cell->car = car;
    cell->cdr = cdr;
    return cell;
}

struct runi_object *runi_acons(struct runi_object *x, struct runi_object *y, struct runi_object *a) {
    return runi_cons(runi_cons(x, y), a);
}
