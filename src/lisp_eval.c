#include "lisp.h"

/**
 * @brief 将 list 元素反向压栈
 * @note 没有将其放在 lisp_list.c 中，是因为仅在此文件中被使用，所以没必要暴露
 */
static void push_reverse_list(value_t l)
{
    if (l == EMPTY_LIST) {
        return;
    }
    push_reverse_list(tail(l));
    push(head(l));
}

/**
 * @brief 顶层求值，先反序压栈，再从最底层网上求值
 */
value_t eval_toplevel(value_t l)
{
    value_t res = NIL;
    assert(g_sp == 0, "g_sp should be 0 when calling eval_toplevel");
    push_reverse_list(l);
    while (g_sp != 0) {
        value_t v = pop();
        res = eval(v);
    }
    return res;
}

static value_t eval_sexp(value_t sexp, bool noeval);

static value_t expand(value_t v)
{
    return eval_sexp(v, true);
}

value_t eval(value_t v)
{
    return eval_sexp(v, false);
}

#define tail_eval(exp) do { sexp = (exp); restore_stack(ss); goto eval_top; } while(0);

static void _assert_nargs(int _nargs, int n)
{
    assert(_nargs == n, "Error: too %s arguments", (_nargs > n ? "many" : "few"));
}
#define assert_nargs(n) _assert_nargs(nargs, (n))

static inline value_t to_bool(value_t v)
{
    return (v == NIL || v == EMPTY_LIST) ? NIL : T;
}

static value_t eqp(value_t v1, value_t v2);
static value_t copy_body(value_t body);
static void prepare_env(value_t args, int ss);
static value_t eval_sym(value_t v);

/**
 * @brief 本文件中最核心的函数
 */
static value_t eval_sexp(value_t sexp, bool noeval)
{
    //printf("\neval_sexp:\n");
    //print(sexp);
    value_t fun, funtype, args, body;
    BuiltinCode code;
    int nargs, sum;
    int ss = g_sp, ee = g_env_sp;

    int tail_macro = 1;

    value_t res = NIL;
    bool is_apply = false;
eval_top:
    if (tail_macro > 0) {
        --tail_macro;
    }
    /* printf("Env:");  dump_env(); */
    /* print(sexp); NL; */
    switch (type_of(sexp)) {
    case TYPE_SYM:
        res = eval_sym(sexp);
        break;

    case TYPE_LIST:
        push(tail(sexp));       //args
        fun = eval(head(sexp));
apply_top:
        //println(fun);
        //println(top());
        if (type_of(fun) == TYPE_BUILTIN) {
            goto apply_builtin;
        }

        if (type_of(fun) == TYPE_LIST) {
            args = (head(tail(fun)));   // 形参
            body = tail(tail(fun));     // 函数体
            goto apply;
        }
        print(fun);
        error("Applying not a function");
        break;
    default:
        res = sexp;
        break;
    }
    goto end;

apply_builtin:
    code = builtin_val(fun)->code;
    args = pop();
    if (code >= F_ADD) {
        push_reverse_list(args);
        if (!is_apply) {
            for (int i = g_sp - 1; i >= ss; --i) {
                g_stack[i] = eval(g_stack[i]);
            }
        }
        is_apply = false;
    }
    nargs = g_sp - ss;

    switch (code) {
    case F_ADD:
        assert(nargs > 0, "Too few arguments");
        sum = num_val(pop());
        while (g_sp > ss) {
            sum += num_val(pop());
        }
        res = number(sum);
        break;
    case F_SUB:
        assert(nargs > 0, "Too few arguments");
        sum = num_val(pop());
        if (nargs == 1) {
            sum = -sum;
        }
        while (g_sp > ss) {
            sum -= num_val(pop());
        }
        res = number(sum);
        break;
    case F_MUL:
        assert(nargs > 0, "Too few arguments");
        sum = num_val(pop());
        while (g_sp > ss) {
            sum *= num_val(pop());
        }
        res = number(sum);
        break;
    case F_DIV:
        assert(nargs > 0, "Too few arguments");
        sum = num_val(pop());
        while (g_sp > ss) {
            number_t num = num_val(pop());
            assert(num != 0, "Division by zero");
            sum /= num;
        }
        res = number(sum);
        break;
    case F_LT:
    {
        assert_nargs(2);
        number_t n1 = num_val(pop());
        number_t n2 = num_val(pop());
        if (n1 < n2) {
            res = T;
        } else {
            res = NIL;
        }
    }
    break;
    case F_GT:
    {
        assert_nargs(2);
        number_t n1 = num_val(pop());
        number_t n2 = num_val(pop());
        if (n1 > n2) {
            res = T;
        }
    }
    break;
    case F_EQ:
    {
        assert_nargs(2);
        value_t v1 = pop();
        value_t v2 = pop();
        res = eqp(v1, v2);
    }
    break;
    case F_NOT:
        assert_nargs(1);
        if (pop() == NIL) {
            res = T;
        } else {
            res = NIL;
        }
        break;
    case F_HEAD:
    {
        assert_nargs(1);
        value_t v = pop();
        assert_type(v, LIST);
        res = head(v);
    }
    break;
    case F_TAIL:
    {
        assert_nargs(1);
        assert_type(g_stack[ss], LIST);
        res = tail(g_stack[ss]);
    }
    break;
    case F_LISTP:
        assert_nargs(1);
        res = type_of(g_stack[ss]) == TYPE_LIST ? T : NIL;
        break;
    case F_SYMBOLP:
        assert_nargs(1);
        res = type_of(g_stack[ss]) == TYPE_SYM ? T : NIL;
        break;
    case F_NUMBERP:
        assert_nargs(1);
        res = type_of(g_stack[ss]) == TYPE_NUM ? T : NIL;
        break;
    case F_BUILTINP:
        assert_nargs(1);
        res = type_of(g_stack[ss]) == TYPE_BUILTIN ? T : NIL;
        break;
    case B_COND:
        push_list(args);
        for (int i = ss; i < g_sp; ++i) {
            value_t pair = g_stack[i];
            value_t cond = head(pair);
            push(head(tail_(pair)));
            res = eval(cond);
            if (to_bool(res) != NIL) {
                if (tail_macro > 0) {
                    ++tail_macro;
                }
                tail_eval(pop());
                break;
            }
            pop();
        }
        break;
    case B_DEF:
    {
        const char* name = sym_val(head(args))->name;
        value_t sym = symbol(name, &symtab);
        println(tail_(args));
        println(head(tail_(args)));
        res = eval(head(tail_(args)));
        println(res);
        sym_val(sym)->binding = res;
    }
    break;
    case B_OR:
        push_reverse_list(args);
        while (g_sp > ss) {
            res = eval(pop());
            if (res != NIL && res != EMPTY_LIST) {
                break;
            }
        }
        break;
    case B_AND:
        push_reverse_list(args);
        while (g_sp > ss) {
            res = eval(pop());
            if (res == NIL) {
                break;
            }
        }
        break;
    case F_PRINT:
        while (g_sp > ss) {
            println(pop());
        }
        break;
    case F_EVAL:
        assert_nargs(1);
        res = eval(g_stack[ss]);
        break;
    case F_APPLY:
    {
        fun = pop();
        is_apply = true;
        goto apply_top;
    }
    break;
    case B_MACRO:
    case B_FN:
    {
        int ee1 = g_env_sp;
        body = tail(args);
        args = head(args);
        push(args);
        // avoid unnessessary replacements while expanding macros
        if (!(code == B_FN && noeval)) {
            if (is_list(args)) {
                // push arg symbols to g_env_stack twice for shadowing
                for (value_t h = args; h != EMPTY_LIST; h = tail(h)) {
                    if (head(h) != REST) {
                        env_push(head(h));
                        env_push(head(h));
                    }
                }
            } else {
                env_push(args);
                env_push(args);
            }
        }
        body = copy_body(body);
        args = pop();
        res = cons_(code == B_FN ? FN : MACRO, cons_(args, body));
        env_restore_stack(ee1);
    }
    break;
    case B_DO:
        push_reverse_list(args);
        while (g_sp > ss) {
            res = eval(pop());
        }
        break;
    case F_CONS:
    {
        assert_nargs(2);
        value_t c1 = pop();
        value_t c2 = pop();
        res = cons(c1, c2);
        break;
    }
    case B_QUOTE:
        assert_nargs(0);
        res = head(args);
        break;
    default:
        error("Unknown builtin %d", code);
    }
    goto end;
apply:
    {
        funtype = head(fun);
        assert(funtype == FN || funtype == MACRO, "Applying not a function!!!!!");
        value_t list = pop();   // 入参
        println(list);
        println(args);          // 形参
        println(body);          // 函数体
        push(body);
        push(args);

        int ss0 = g_sp;
        println(list);
        push_list(list);
        for (int i = ss0; i < g_sp; ++i) {
            if (funtype != MACRO && !is_apply) {
                g_stack[i] = eval(g_stack[i]);
            }
        }

        is_apply = false;
        args = g_stack[ss0 - 1];
        env_restore_stack(ee);

        prepare_env(args, ss0); // argnames
        args = pop();
        body = pop();

        println(args);          // 形参
        println(body);          // 函数体

        push_reverse_list(body);

        while (g_sp > ss) {
            if (g_sp == ss + 1) {
                if (funtype == MACRO) {
                    tail_macro += 2;
                } else {
                    if (tail_macro) {
                        tail_macro++;
                    }
                }
                tail_eval(pop());
            } else {
                res = eval(pop());
            }
        }
    }
end:
    if (tail_macro && !noeval) {
        tail_eval(res);
    }
    restore_stack(ss);
    env_restore_stack(ee);
    return res;
}

/**
 * @brief 判断两个值是否一致（而非内存一致）
 */
static value_t eqp(value_t v1, value_t v2)
{
    if (type_of(v1) != TYPE_LIST || type_of(v2) != TYPE_LIST) {
        return v1 == v2 ? T : NIL;
    }

    while (v1 != EMPTY_LIST && v2 != EMPTY_LIST) {
        if (!eqp(head(v1), head(v2))) {
            return NIL;
        }
        v1 = tail(v1);
        v2 = tail(v2);
    }
    return v1 == v2 ? T : NIL;
}

/**
 * 
 * @brief 闭包创建时的变量捕获函数
 * 当解释器执行 fn 或 macro 时调用，它深拷贝函数体
 * 同时将当前环境中绑定的自由变量替换为实际值 —实现深拷贝捕获语义
 */
static value_t copy_body(value_t body)
{
    int ss = g_sp;
    //int sum, nargs;
    switch (type_of(body)) {
    case TYPE_LIST:
        if (body == EMPTY_LIST || head(body) == QUOTE) {
            return body;
        }
        // check if it if a macro
        if (is_sym(head(body))) {
            push(body);
            value_t tmp = eval(head(body));

            body = pop();

            if (is_list(tmp) && head(tmp) == MACRO) {
                body = expand(body);
                return copy_body(body);
            }
        }
        // optimisation for not doing unnessessary copies
        if (g_env_sp == 0) {
            return body;
        }

        push_list(body);
        for (int i = ss; i < g_sp; ++i) {
            g_stack[i] = copy_body(g_stack[i]);
        }
        return pop_list(ss);

    case TYPE_SYM:
        // replace symbol from its value from environment
        for (int i = g_env_sp - 1; i > 0; i -= 2) {
            if (g_env_stack[i] == body) {
                return g_env_stack[i - 1];
            }
        }
        return body;

    default:
        return body;
    }
}

static void prepare_env(value_t args, int ss)
{
    println(args);
    int sp = ss;
    if (is_list(args)) {
        for (value_t h = args; h != EMPTY_LIST; h = tail(h)) {
            if (head(h) != REST) {
                if (ss == g_sp) {
                    error("Not enough args");
                }
                env_push(g_stack[ss]);
                env_push(head(h));
                ++ss;
            } else {
                args = head(tail(h));
                break;
            }
        }
    }

    // NOTE: 注意在前面的判断中, args 可能被改变 [陈智鹏@2026-6-29]
    if (!is_list(args)) {
        env_push(pop_list(ss));
        env_push(args);
    }

    restore_stack(sp);
}

/**
 * @note 环境栈 g_env_stack 的布局是 [值, 符号名, 值, 符号名, ...]，每对是 [value, symbol]。遇到符号时倒序查找：
 * 找到 → 返回符号当前绑定的值（替换为实际值，完成捕获）
 * 没找到 → 返回符号本身（自由变量，运行时在全局环境查找）
 */
static value_t eval_sym(value_t v)
{
#if 0
    if (v == UNBOUND) {
        error("Cannot eval unbound");
    }
#endif

    for (int i = g_env_sp - 1; i >= 0; i -= 2) {
        if (g_env_stack[i] == v) {
            //printf("Sym in g_env_stack:"); print(v); print(g_env_stack[i-1]); NL;
            return g_env_stack[i - 1];
        }
    }

    Symbol* sym = _symbol(sym_val(v)->name, &symtab);
    UNUSED(sym);

    return sym_val(v)->binding;
}

// 未被使用
#if 0
static void prepare_args(value_t args)
{
    int ss = g_sp;
    push_list(args);

    for (int i = ss; i < g_sp; ++i) {
        g_stack[i] = eval(g_stack[i]);
    }
}
#endif
