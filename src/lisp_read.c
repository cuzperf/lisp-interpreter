#include "lisp.h"

static inline bool is_space(char c)
{
    return c == ' ' || c == '\t' || c == '\n';
}

// 从文件中取一个 char，然后回退指针，就好像没取一样
static inline char fpeekc(FILE* f)
{
    char c = (char)getc(f);
    /* printf("%c\n", c); */
    ungetc(c, f);   // 把上次取出来的再退回去（之前都没用过！）
    return c;
}

static void skip_spaces(FILE* f)
{
    char c = (char)fgetc(f);
    while (is_space(c)) {
        c = (char)getc(f);
    }
    ungetc(c, f);
}

#define MAX_NAME (256)
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

static void read_int(FILE* f, Symbol** env)
{
    UNUSED(env);
    int n = 0;
    char c = (char)fgetc(f);
    while (!is_space(c) && c != ')' && c != '(') {
        n = 10 * n + (c - 0x30);
        c = (char)fgetc(f);
    }
    ungetc(c, f);
    push(number(n));
}

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

// 读取文本，将符号记录到符号表 symtab 中，将读取的内容记录在 g_stack 中
void read(FILE* f, Symbol** env)
{
    char c;
start:
    c = fpeekc(f);
    switch (c) {
    case '(':
        read_list(f, env);
        break;
    case ' ': case '\n': case '\t':
        skip_spaces(f);
        goto start;
        return;
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
        while (c != '\n') {
            c = (char)getc(f);
        }
        //goto start;
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
        if (ss1 != g_sp) {
            value_t tmp = make_cell(UNBOUND);
            head(tmp) = pop();
            push(tmp);
        }
    }
    fclose(f);

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
