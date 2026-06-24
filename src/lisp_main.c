#include "lisp.h"

int main(int argc, char* argv[])
{
    lisp_init();
    value_t sexp = read_file("system.lsp");
    eval_toplevel(sexp);

    if (argc > 1) {
        // 纯文件名或全路径文件名
        value_t user_sexpr = read_file(argv[1]);
        eval_toplevel(user_sexpr);
    } else {
        lisp_repl();
    }
    return 0;
}
