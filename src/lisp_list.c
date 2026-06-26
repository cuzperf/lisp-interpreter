#include "lisp.h"

/**
 * 本文件对外提供五个接口函数 cons_, cons, push_list, pop_list, make_list
 */

 // NOTE: 这里的 List 除了 EMPTY_LIST 外都是堆上的元素 [陈智鹏@2026-6-26]
 /*  (a (b c) d)
 *
 *  [a|●]--→[●|●]──→[d|EMPTY_LIST]
 *             |
 *             |
 *            [b|●]──→[c|EMPTY_LIST]
 */

/**
 * @brief 创建 (h . t) 的 List 单元（不要求 t 为 List ）
 * @note 注意 make_cell 可能会触发 gc 从而导致 h 和 t 指向的内容失效
 */
value_t cons_(value_t h, value_t t)
{
    push(h);
    push(t);
    value_t cell = make_cell(UNBOUND);
    tail_(cell) = pop();
    head_(cell) = pop();
    return cell;
}

/**
 * @brief 创建 (h . t) 的 List 单元（要求 t 为 List ）
 */
value_t cons(value_t h, value_t t)
{
    assert_type(t, LIST);
    return cons_(h, t);
}

/**
 * @brief 将 List 逐元素压栈
 */
void push_list(value_t l)
{
    for (value_t v = l; v != EMPTY_LIST; v = tail_(v)) {
        assert_type(v, LIST);
        push(head_(v));
    }
}

/**
 * @brief 逐个出栈直到 g_sp 等于 ss
 * @return 以 g_stack[ss] 为顶元素的 List
 */
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

/**
 * @brief 将可变参数个元素，构建成一个以 h 为顶元素的 List
 */
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
