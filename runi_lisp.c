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

static int runi_peek(void) {
    int c = getchar();
    ungetc(c, stdin);
    return c;
}

static void skip_line(void) {
    for (;;) {
        int c = getchar();
        if (c == EOF || c == '\n')
            return;
        if (c == '\r') {
            if (runi_peek() == '\n')
                getchar();
            return;
        }
    }
}

static struct runi_object *parse_list(void) {
    struct runi_object *obj = runi_parse();
    if (!obj)
        runi_error("Unclosed parenthesis");
    if (obj == runi_dot)
        runi_error("Stray dot");
    if (obj == runi_cparen)
        return runi_nil;
    struct runi_object *head, *tail;
    head = tail = runi_cons(obj, runi_nil);

    for (;;) {
        struct runi_object *obj = runi_parse();
        if (!obj)
            runi_error("Unclosed parenthesis");
        if (obj == runi_cparen)
            return head;
        if (obj == runi_dot) {
            tail->cdr = runi_parse();
            if (runi_parse() != runi_cparen)
                runi_error("Closed parenthesis expected after dot");
            return head;
        }
        tail->cdr = runi_cons(obj, runi_nil);
        tail = tail->cdr;
    }
}

struct runi_object *runi_intern(char *name) {
    for (struct runi_object *p = runi_symbols; p != runi_nil; p = p->cdr)
        if (strcmp(name, p->car->name) == 0)
            return p->car;
    struct runi_object *sym = runi_make_symbol(name);
    runi_symbols = runi_cons(sym, runi_symbols);
    return sym;
}

static struct runi_object *parse_quote(void) {
    struct runi_object *sym = runi_intern("quote");
    return runi_cons(sym, runi_cons(runi_parse(), runi_nil));
}

static int parse_number(int val) {
    while (isdigit(runi_peek()))
        val = val * 10 + (getchar() - '0');
    return val;
}

static struct runi_object *parse_symbol(char c) {
    char buf[RUNI_SYMBOL_MAX_LEN + 1];
    int len = 1;
    buf[0] = c;
    while (isalnum(runi_peek()) || runi_peek() == '-') {
        if (RUNI_SYMBOL_MAX_LEN <= len)
            runi_error("Symbol name too long");
        buf[len++] = getchar();
    }
    buf[len] = '\0';
    return runi_intern(buf);
}

struct runi_object *runi_parse(void) {
    for (;;) {
        int c = getchar();
        if (c == ' ' || c == '\n' || c == '\r' || c == '\t')
            continue;
        if (c == EOF)
            return NULL;
        if (c == ';') {
            skip_line();
            continue;
        }
        if (c == '(')
            return parse_list();
        if (c == ')')
            return runi_cparen;
        if (c == '.')
            return runi_dot;
        if (c == '\'')
            return parse_quote();
        if (isdigit(c))
            return runi_make_integer(parse_number(c - '0'));
        if (c == '-')
            return runi_make_integer(-parse_number(0));
        if (isalpha(c) || strchr("+=!@#$%^&*", c))
            return parse_symbol(c);
        runi_error("Don't know how to handle %c", c);
    }
}
