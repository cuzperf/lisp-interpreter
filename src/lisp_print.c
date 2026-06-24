#include "lisp.h"

static void print_list(value_t v)
{
    if (v == EMPTY_LIST) {
        printf("()");
        return;
    }

    if (head(v) == QUOTE) {
        printf("'");
        if (tail(v) != EMPTY_LIST) {
            print(head(tail(v)));
        }
        return;
    }

    if (head(v) == QUASIQUOTE) {
        printf("`");
        if (tail(v) != EMPTY_LIST) {
            print(head(tail(v)));
        }
        return;
    }

    if (head(v) == UNQUOTE) {
        printf(",");
        if (tail(v) != EMPTY_LIST) {
            print(head(tail(v)));
        }
        return;
    }

    if (head(v) == UNQUOTE_SPLICING) {
        printf(",@");
        if (tail(v) != EMPTY_LIST) {
            print(head(tail(v)));
        }
        return;
    }

    printf("(");

    if (head(v) == RELOCATED_MARK) {
        printf("Relocated");
        print(tail(v));
        return;
    }

    while (1) {
        print(head(v));
        v = tail(v);
        if (v == EMPTY_LIST) {
            break;
        }
        printf(" ");
    }
    printf(")");
}

void print(value_t v)
{
    switch (type_of(v)) {
    case TYPE_LIST:
        print_list(v);
        break;
    case TYPE_NUM:
        printf("%d", num_val(v));
        break;
    case TYPE_SYM:
        if (v == UNBOUND) {
            printf("unbound");
        } else {
            printf("%s", sym_val(v)->name);
        }
        break;
    case TYPE_BUILTIN:
        printf("#<builtin %s >", builtin_names[builtin_val(v)->code]);
        break;
    default:
        printf("default");
        break;
    }
}
