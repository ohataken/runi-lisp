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
