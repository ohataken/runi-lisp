#include "runi_lisp.h"

#include <stdio.h>

int main(void) {
    printf("runi-lisp\n");

    runi_nil = runi_make_special(RUNI_NIL);
    runi_dot = runi_make_special(RUNI_DOT);
    runi_cparen = runi_make_special(RUNI_CPAREN);
    runi_true = runi_make_special(RUNI_TRUE);
    runi_symbols = runi_nil;

    struct runi_object *env = runi_make_env(runi_nil, NULL);

    runi_add_variable(env, runi_intern("t"), runi_true);
    runi_add_primitive(env, "quote", runi_prim_quote);
    runi_add_primitive(env, "list", runi_prim_list);
    runi_add_primitive(env, "setq", runi_prim_setq);
    runi_add_primitive(env, "+", runi_prim_plus);
    runi_add_primitive(env, "define", runi_prim_define);
    runi_add_primitive(env, "defun", runi_prim_defun);
    runi_add_primitive(env, "defmacro", runi_prim_defmacro);
    runi_add_primitive(env, "macroexpand", runi_prim_macroexpand);
    runi_add_primitive(env, "lambda", runi_prim_lambda);
    runi_add_primitive(env, "if", runi_prim_if);
    runi_add_primitive(env, "=", runi_prim_num_eq);
    runi_add_primitive(env, "println", runi_prim_println);
    runi_add_primitive(env, "exit", runi_prim_exit);

    for (;;) {
        struct runi_object *expr = runi_parse();
        if (!expr)
            return 0;
        if (expr == runi_cparen)
            runi_error("Stray close parenthesis");
        if (expr == runi_dot)
            runi_error("Stray dot");
        runi_print(runi_eval(env, expr));
        printf("\n");
    }

    return 0;
}
