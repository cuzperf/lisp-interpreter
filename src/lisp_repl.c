#include "lisp.h"
#include <setjmp.h>

// for error handling in REPL
static jmp_buf jmp_mark;
static bool in_repl = false;

void fail()
{
    if (in_repl) {
        longjmp(jmp_mark, -1);
    } else {
        exit(1);
    }
}

void lisp_repl()
{
    in_repl = true;
    while (true) {
        NL;
        printf(">");
        int ss = g_sp;
        int ee = g_env_sp;

        if (!setjmp(jmp_mark)) {
            read(stdin, &symtab);
            if (ss != g_sp) {
                value_t res = eval(pop());
                print(res);
            }
        } else {
            restore_stack(ss);
            env_restore_stack(ee);
        }
    }
}
