#include "lisp.h"

void push_list(value_t l)
{
    for (value_t v = l; v != EMPTY_LIST; v = tail_(v)) {
        push(head_(v));
    }
}

value_t pop_list(int ss)
{
    push(EMPTY_LIST);
    while (g_sp > ss + 1) {
        value_t t = pop();
        value_t h = pop();
        push(cons_(h, t));
    }
    return pop();
}

value_t make_list(value_t h, ...)
{
    va_list args;
    int ss = g_sp;
    va_start(args, h);

    for (value_t v = h; v != END; v = va_arg(args, value_t)) {
        push(v);
    }

    va_end(args);
    return pop_list(ss);
}
