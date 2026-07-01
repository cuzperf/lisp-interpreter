#include "lisp.h"

/**
 * 本文件对外提供两个接口函数 read_file 和 read
 */

// TODO: 补充函数 read_string 读取完整的 lisp 语句，然后进行执行，
//       即可获得动态解析字符串的能力 [陈智鹏@2026-6-25]
// void read_string(const char* lispString,  Symbol** env);

// TODO: 补充函数 read_double （此时需要补充属性位了）[陈智鹏@2026-6-25]

static inline bool is_space(char c)
{
    return c == ' ' || c == '\t' || c == '\n';
}

// 从文件中取一个 char, 然后回退指针，就好像没取一样
static inline char fpeekc(FILE* f)
{
    char c = (char)getc(f);
    /* printf("%c\n", c); */
    ungetc(c, f);   // 把上次取出来的再退回去（之前都没用过！）
    return c;
}

// 忽略空白字符，读到最后一个非空白字符后，再回退回去
static void skip_spaces(FILE* f)
{
    char c = (char)fgetc(f);
    while (is_space(c)) {
        c = (char)getc(f);
    }
    ungetc(c, f);
}

static void skip_comments(FILE* f)
{
    char c = (char)fgetc(f);
    while (c != EOF && c != '\n') {
        c = (char)getc(f);
    }
    ungetc(c, f);
}

#define MAX_NAME (256)
/**
 * @note 向符号环境中补充一个符号（如果符号环境中没有）
 * @note 向 g_stack 中加入一个元素
 */
static void read_sym(FILE* f, Symbol** env)
{
    char c;
    char buf[MAX_NAME];
    int i = 0;
    do {
        c = (char)getc(f);
        buf[i] = c;
        c = fpeekc(f);
        i++;
        if (i > MAX_NAME) {
            error("name too long");
        }
    } while (!is_space(c) && c != ')' && c != '(' && !feof(f));
    buf[i] = '\0';
    push(symbol(buf, env));
}

/**
 * @note 向 g_stack 中加入一个元素
 */
static void read_int(FILE* f, Symbol** env)
{
    UNUSED(env);
    int n = 0;
    char c = (char)fgetc(f);
    while (c >= '0' && c <= '9') {
        n = 10 * n + (c - 0x30);
        c = (char)fgetc(f);
    }
    ungetc(c, f);
    push(number(n));
}

/**
 * @note 向 g_stack 中加入一个元素（即使 EMPTY_LIST 也如此）
 */
static void read_list(FILE* f, Symbol** env)
{
    char c;
    getc(f);
    c = (char)fpeekc(f);
    if (c == ')') {
        getc(f);
        push(EMPTY_LIST);
        return;
    }

    int ss = g_sp;
    while (!feof(f)) {
        c = (char)fpeekc(f);
        if (c == ')') {
            getc(f);
            break;
        }
        if (c == ';') {
            skip_comments(f);
            continue;
        }
        if (is_space(c)) {
            skip_spaces(f);
            continue;
        }
        read(f, env);
    }
    push(pop_list(ss));
    /* skip_spaces(f); */
    /* printf("%c\n", fpeekc(f)); */
}

/**
* @brief 读取文件，向 g_stack 中加入一个元素（读取的内容）
* @note 中间产生的符号记录在符号环境中
* @note 检查所有的分支，可归纳法得出 read 向 g_stack 中加入一个元素
*/
void read(FILE* f, Symbol** env)
{
    char c;
start:
    c = fpeekc(f);
    switch (c) {
    case EOF:
        push(NIL);
        break;
    case '(':
        read_list(f, env);
        break;
    case ' ': case '\n': case '\t':
        skip_spaces(f);
        goto start;
        break;
    case '\'':
        getc(f);
        read(f, env);
        push(make_list(QUOTE, pop(), END));
        break;
    case ',':
    {
        getc(f);
        char cc = fpeekc(f);
        value_t ut = UNQUOTE;
        if (cc == '@') {
            fgetc(f);
            ut = UNQUOTE_SPLICING;
        }
        read(f, env);
        push(make_list(ut, pop(), END));
    }
    break;
    case '`':
        getc(f);
        read(f, env);
        push(make_list(QUASIQUOTE, pop(), END));
        break;
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
        read_int(f, env);
        break;
    case ';':
        skip_comments(f);
        goto start;
        break;
    case ')':
        getc(f);
        error("Unmatched closing parentesis");
        break;
    default:
        read_sym(f, env);
        break;
    }
}

/**
 * @brief 打开 name 文件，调用 read 函数，处理 g_stack 中内容变成一个 List 作为返回值
 * @note 最终 g_stack 和 g_sp 无任何变化
 */
value_t read_file(const char* name)
{
    FILE* f = fopen(name, "rt");
    if (f == NULL) {
        return UNBOUND;
    }
    int ss = g_sp;
    while (!feof(f)) {
        int ss1 = g_sp;
        read(f, &symtab);
        assert(ss1 == g_sp - 1, "read will push exact one element!");
        value_t tmp = make_cell(UNBOUND);
        head(tmp) = pop();
        push(tmp);
    }
    fclose(f);

    // NOTE: 从 ss 到 g_sp 的元素都是堆上的元素，故而下面操作是安全的 [陈智鹏@2026-6-26]
    int ss1 = ss + 1;
    value_t cur = g_stack[ss];
    while (ss1 < g_sp) {
        tail(cur) = g_stack[ss1];
        cur = g_stack[ss1];
        ss1++;
    }
    restore_stack(ss + 1);
    return pop();
}
