#include "lisp.h"

void dump_symtab(Symbol* s)
{
    printf("%s :", s->name);
    print(s->binding);
    NL;
    if (s->left) {
        printf(" ");
        dump_symtab(s->left);
    }
    if (s->right) {
        printf(" ");
        dump_symtab(s->right);
    }
}

void dump_heap()
{
    memory_t tmp = g_heap;
    while (tmp < g_curheap - sizeof(List)) {
        value_t val = ((List*)tmp)->head;
        print(val);
        printf(" -|- ");
        tmp += sizeof(List);
    }
}

void dump_stack()
{
    int cur = 0;
    while (cur < g_sp) {
        print(g_stack[cur]);
        cur++;
        printf("\t");
    }
    printf("\n");
}

void dump_env()
{
    int cur = 0;
    while (cur < g_env_sp) {
        print(g_env_stack[cur]);
        cur++;
        printf("\t");
    }
    printf("\n");
}
