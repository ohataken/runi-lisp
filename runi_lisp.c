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

struct runi_object *runi_make_string(char *string) {
    struct runi_object *str = runi_alloc(RUNI_STRING, strlen(string) + 1);
    strcpy(str->string, string);
    return str;
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

void runi_print(struct runi_object *obj) {
    switch (obj->type) {
    case RUNI_INTEGER:
        printf("%d", obj->integer);
        return;
    case RUNI_LIST:
        printf("(");
        for (;;) {
            runi_print(obj->car);
            if (obj->cdr == runi_nil)
                break;
            if (obj->cdr->type != RUNI_LIST) {
                printf(" . ");
                runi_print(obj->cdr);
                break;
            }
            printf(" ");
            obj = obj->cdr;
        }
        printf(")");
        return;
    case RUNI_SYMBOL:
        printf("%s", obj->name);
        return;
    case RUNI_STRING:
        printf("%s", obj->string);
        return;
    case RUNI_PRIMITIVE:
        printf("<primitive>");
        return;
    case RUNI_FUNCTION:
        printf("<function>");
        return;
    case RUNI_MACRO:
        printf("<macro>");
        return;
    case RUNI_NIL:
    case RUNI_TRUE:
        if (obj == runi_nil)
            printf("()");
        else if (obj == runi_true)
            printf("t");
        return;
    default:
        runi_error("Bug: print: Unknown tag type: %d", obj->type);
    }
}

void runi_add_variable(struct runi_object *env, struct runi_object *sym, struct runi_object *val) {
    env->vars = runi_acons(sym, val, env->vars);
}

int runi_list_length(struct runi_object *list) {
    int len = 0;
    for (;;) {
        if (list == runi_nil)
            return len;
        if (list->type != RUNI_LIST)
            runi_error("length: cannot handle dotted list");
        list = list->cdr;
        len++;
    }
}

static struct runi_object *runi_push_env(struct runi_object *env, struct runi_object *vars, struct runi_object *values) {
    if (runi_list_length(vars) != runi_list_length(values))
        runi_error("Cannot apply function: number of argument does not match");
    struct runi_object *map = runi_nil;
    for (struct runi_object *p = vars, *q = values; p != runi_nil; p = p->cdr, q = q->cdr) {
        struct runi_object *sym = p->car;
        struct runi_object *val = q->car;
        map = runi_acons(sym, val, map);
    }
    return runi_make_env(map, env);
}

struct runi_object *runi_progn(struct runi_object *env, struct runi_object *list) {
    struct runi_object *r = NULL;
    for (struct runi_object *lp = list; lp != runi_nil; lp = lp->cdr)
        r = runi_eval(env, lp->car);
    return r;
}

struct runi_object *runi_eval_list(struct runi_object *env, struct runi_object *list) {
    struct runi_object *head = NULL;
    struct runi_object *tail = NULL;
    for (struct runi_object *lp = list; lp != runi_nil; lp = lp->cdr) {
        struct runi_object *tmp = runi_eval(env, lp->car);
        if (head == NULL) {
            head = tail = runi_cons(tmp, runi_nil);
        } else {
            tail->cdr = runi_cons(tmp, runi_nil);
            tail = tail->cdr;
        }
    }
    if (head == NULL)
        return runi_nil;
    return head;
}

bool runi_is_list(struct runi_object *obj) {
  return obj == runi_nil || obj->type == RUNI_LIST;
}

static struct runi_object *runi_apply(struct runi_object *env, struct runi_object *fn, struct runi_object *args) {
    if (!runi_is_list(args))
        runi_error("argument must be a list");
    if (fn->type == RUNI_PRIMITIVE)
        return fn->fn(env, args);
    if (fn->type == RUNI_FUNCTION) {
        struct runi_object *body = fn->body;
        struct runi_object *args = fn->args;
        struct runi_object *eargs = runi_eval_list(env, args);
        struct runi_object *newenv = runi_push_env(fn->env, args, eargs);
        return runi_progn(newenv, body);
    }
    runi_error("not supported");
}

struct runi_object *runi_find(struct runi_object *env, struct runi_object *sym) {
    for (struct runi_object *p = env; p; p = p->parent) {
        for (struct runi_object *cell = p->vars; cell != runi_nil; cell = cell->cdr) {
            struct runi_object *bind = cell->car;
            if (sym == bind->car)
                return bind;
        }
    }
    return NULL;
}

struct runi_object *runi_macroexpand(struct runi_object *env, struct runi_object *obj) {
    if (obj->type != RUNI_LIST || obj->car->type != RUNI_SYMBOL)
        return obj;
    struct runi_object *bind = runi_find(env, obj->car);
    if (!bind || bind->cdr->type != RUNI_MACRO)
        return obj;
    struct runi_object *args = obj->cdr;
    struct runi_object *body = bind->cdr->body;
    struct runi_object *params = bind->cdr->args;
    struct runi_object *newenv = runi_push_env(env, params, args);
    return runi_progn(newenv, body);
}

struct runi_object *runi_eval(struct runi_object *env, struct runi_object *obj) {
    switch (obj->type) {
    case RUNI_INTEGER:
    case RUNI_PRIMITIVE:
    case RUNI_FUNCTION:
    case RUNI_NIL:
    case RUNI_DOT:
    case RUNI_TRUE:
    case RUNI_STRING:    
        return obj;
    case RUNI_SYMBOL: {
        struct runi_object *bind = runi_find(env, obj);
        if (!bind)
            runi_error("Undefined symbol: %s", obj->name);
        return bind->cdr;
    }
    case RUNI_LIST: {
        struct runi_object *expanded = runi_macroexpand(env, obj);
        if (expanded != obj)
            return runi_eval(env, expanded);
        struct runi_object *fn = runi_eval(env, obj->car);
        struct runi_object *args = obj->cdr;
        if (fn->type != RUNI_PRIMITIVE && fn->type != RUNI_FUNCTION)
            runi_error("The head of a list must be a function");
        return runi_apply(env, fn, args);
    }
    default:
        runi_error("Bug: eval: Unknown tag type: %d", obj->type);
    }
}
struct runi_object *runi_prim_quote(struct runi_object *env, struct runi_object *list) {
    if (runi_list_length(list) != 1)
        runi_error("Malformed quote");
    return list->car;
}

struct runi_object *runi_prim_list(struct runi_object *env, struct runi_object *list) {
    return runi_eval_list(env, list);
}

struct runi_object *runi_prim_setq(struct runi_object *env, struct runi_object *list) {
    if (runi_list_length(list) != 2 || list->car->type != RUNI_SYMBOL)
        runi_error("Malformed setq");
    struct runi_object *bind = runi_find(env, list->car);
    if (!bind)
        runi_error("Unbound variable %s", list->car->name);
    struct runi_object *value = runi_eval(env, list->cdr->car);
    bind->cdr = value;
    return value;
}

struct runi_object *runi_prim_plus(struct runi_object *env, struct runi_object *list) {
    int sum = 0;
    for (struct runi_object *args = runi_eval_list(env, list); args != runi_nil; args = args->cdr) {
        if (args->car->type != RUNI_INTEGER)
            runi_error("+ takes only numbers");
        sum += args->car->integer;
    }
    return runi_make_integer(sum);
}

static struct runi_object *runi_handle_function(struct runi_object *env, struct runi_object *list, int type) {
    if (list->type != RUNI_LIST || !runi_is_list(list->car) || list->cdr->type != RUNI_LIST)
        runi_error("Malformed lambda");
    for (struct runi_object *p = list->car; p != runi_nil; p = p->cdr) {
        if (p->car->type != RUNI_SYMBOL)
            runi_error("Parameter must be a symbol");
        if (!runi_is_list(p->cdr))
            runi_error("Parameter list is not a flat list");
    }
    struct runi_object *car = list->car;
    struct runi_object *cdr = list->cdr;
    return runi_make_function(type, car, cdr, env);
}

struct runi_object *runi_prim_lambda(struct runi_object *env, struct runi_object *list) {
    return runi_handle_function(env, list, RUNI_FUNCTION);
}

struct runi_object *runi_handle_defun(struct runi_object *env, struct runi_object *list, int type) {
    if (list->car->type != RUNI_SYMBOL || list->cdr->type != RUNI_LIST)
        runi_error("Malformed defun");
    struct runi_object *sym = list->car;
    struct runi_object *rest = list->cdr;
    struct runi_object *fn = runi_handle_function(env, rest, type);
    runi_add_variable(env, sym, fn);
    return fn;
}

struct runi_object *runi_prim_defun(struct runi_object *env, struct runi_object *list) {
    return runi_handle_defun(env, list, RUNI_FUNCTION);
}

struct runi_object *runi_prim_define(struct runi_object *env, struct runi_object *list) {
    if (runi_list_length(list) != 2 || list->car->type != RUNI_SYMBOL)
        runi_error("Malformed setq");
    struct runi_object *sym = list->car;
    struct runi_object *value = runi_eval(env, list->cdr->car);
    runi_add_variable(env, sym, value);
    return value;
}

struct runi_object *runi_prim_defmacro(struct runi_object *env, struct runi_object *list) {
    return runi_handle_defun(env, list, RUNI_MACRO);
}

struct runi_object *runi_prim_macroexpand(struct runi_object *env, struct runi_object *list) {
    if (runi_list_length(list) != 1)
        runi_error("Malformed macroexpand");
    struct runi_object *body = list->car;
    return runi_macroexpand(env, body);
}

struct runi_object *runi_prim_println(struct runi_object *env, struct runi_object *list) {
    runi_print(runi_eval(env, list->car));
    printf("\n");
    return runi_nil;
}

struct runi_object *runi_prim_if(struct runi_object *env, struct runi_object *list) {
    if (runi_list_length(list) < 2)
        runi_error("Malformed if");
    struct runi_object *cond = runi_eval(env, list->car);
    if (cond != runi_nil) {
        struct runi_object *then = list->cdr->car;
        return runi_eval(env, then);
    }
    struct runi_object *els = list->cdr->cdr;
    return els == runi_nil ? runi_nil : runi_progn(env, els);
}

struct runi_object *runi_prim_num_eq(struct runi_object *env, struct runi_object *list) {
    if (runi_list_length(list) != 2)
        runi_error("Malformed =");
    struct runi_object *values = runi_eval_list(env, list);
    struct runi_object *x = values->car;
    struct runi_object *y = values->cdr->car;
    if (x->type != RUNI_INTEGER || y->type != RUNI_INTEGER)
        runi_error("= only takes numbers");
    return x->integer == y->integer ? runi_true : runi_nil;
}

struct runi_object *runi_prim_exit(struct runi_object *env, struct runi_object *list) {
    exit(EXIT_SUCCESS);
}

void runi_add_primitive(struct runi_object *env, char *name, runi_primitive *fn) {
    struct runi_object *sym = runi_intern(name);
    struct runi_object *prim = runi_make_primitive(fn);
    runi_add_variable(env, sym, prim);
}
