#include "lisp.h"

int main(int argc, char* argv[])
{
    lisp_init();
    // NOTE: 工作路径必须是 system.lsp 所在路径 [陈智鹏@2026-6-27]
    value_t sexp = read_file("system.lsp");
    eval_toplevel(sexp);

    if (argc > 1) {
        // 纯文件名或全路径文件名
        value_t user_sexpr = read_file(argv[1]);
        //smprint(user_sexpr);
        eval_toplevel(user_sexpr);
    } else {
        lisp_repl();
    }
    return 0;
}
