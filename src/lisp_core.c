#include "lisp.h"

#define INITIAL_HEAP_SIZE (64000)
#define STACK_SIZE (160 * 1024)
#define ENV_SIZE (160 * 1024)
#define HEAP_RESIZE_RATIO (2.0)

Builtin g_builtins[N_BUILTINS];

const char* builtin_names[N_BUILTINS + 1] = {
#define XX(symbol, name) name,
    CL_BUILTIN_FUNCTIONS(XX)
#undef XX
};

value_t FN, MACRO, NIL, T, QUOTE, REST, UNQUOTE, QUASIQUOTE, UNQUOTE_SPLICING;

int g_heap_size;
memory_t g_heap, g_curheap;
static memory_t g_newheap, g_lim, g_gc_tresh;

Symbol* symtab = NULL;

static int g_stack_size = STACK_SIZE;
static int g_env_stack_size = ENV_SIZE;

void lisp_init()
{
    for (int i = 0; i < N_BUILTINS; ++i) {
        g_builtins[i].type = TYPE_BUILTIN;
        g_builtins[i].code = (BuiltinCode)i;
        Symbol* tmp = _symbol(builtin_names[i], &symtab);
        tmp->binding = tagptr(g_builtins + i, TAG_OTHER);
    }

    g_curheap = g_heap = malloc(INITIAL_HEAP_SIZE);
    g_lim = g_heap + INITIAL_HEAP_SIZE;
    g_heap_size = INITIAL_HEAP_SIZE;
    g_newheap = NULL;

    g_stack = malloc(g_stack_size * sizeof(value_t));
    g_env_stack = malloc(g_env_stack_size * sizeof(value_t));

    MACRO = symbol("macro", &symtab);
    FN = symbol("fn", &symtab);
    QUOTE = symbol("quote", &symtab);

    NIL = symbol("nil", &symtab);
    sym_val(NIL)->binding = NIL;

    T = symbol("#t", &symtab);
    sym_val(T)->binding = T;

    QUASIQUOTE = symbol("quasiquote", &symtab);
    UNQUOTE = symbol("unquote", &symtab);
    UNQUOTE_SPLICING = symbol("unquote-splicing", &symtab);
    REST = symbol("&", &symtab);
    sym_val(REST)->binding = REST;
}

type_t type_of(value_t v)
{
    type_t t = tag(v);
    if (t < TAG_OTHER) {
        return t;
    }
    void* p = ptr(v);           // TODO rise error on null
    return ((Type*)p)->type;
}


// Õ»
value_t* g_stack;
int g_sp = 0;

void push(value_t v)
{
    if (g_sp >= g_stack_size) {
        error("Stack overflow");
    }
    g_stack[g_sp] = v;
    ++g_sp;
}

value_t top()
{
    return g_stack[g_sp - 1];
}

value_t pop()
{
    return g_stack[--g_sp];
}

value_t popn(int n)
{
    g_sp -= n;
    return g_stack[g_sp];
}

void restore_stack(int n)
{
    g_sp = n;
}

// »·¾³Õ»
value_t* g_env_stack;
int g_env_sp = 0;

void env_push(value_t v)
{
    if (g_env_sp >= g_env_stack_size - 1) {
        error("Env overflow");
    }
    g_env_stack[g_env_sp] = v;
    g_env_sp++;
}

value_t env_top()
{
    return g_env_stack[g_env_sp - 1];
}

value_t env_pop()
{
    return g_env_stack[--g_env_sp];
}

void env_restore_stack(int n)
{
    g_env_sp = n;
}

// ¶ÑºÍ gc
static void* halloc(size_t);
value_t make_cell(value_t v)
{
    push(v);
    List* cell = halloc(sizeof(List));
    cell->head = pop();
    cell->tail = EMPTY_LIST;
    return tagptr(cell, TAG_LIST);
}

static value_t relocate(value_t);
static void relocate_symtab(Symbol*);

bool is_gc = 0;
void gc()
{
    if (is_gc) {
        error("Gc in gc!!!!");
    }
    is_gc = true;

    int oh = g_heap_size;
    g_heap_size = (int)(g_heap_size * HEAP_RESIZE_RATIO);
    if (g_newheap == NULL) {
        g_newheap = malloc(g_heap_size);
    } else {
        g_newheap = realloc(g_newheap, g_heap_size);
    }

    memory_t t = g_heap;
    g_heap = g_newheap;
    g_newheap = t;

    g_curheap = g_heap;
    g_lim = g_heap + g_heap_size;
    int ss = 0;

    //printf("before:"); dump_stack();
    while (ss < g_sp) {
        g_stack[ss] = relocate(g_stack[ss]);
        ss++;
    }
    int ee = g_env_sp - 2;

    while (ee >= 0) {
        if (type_of(g_env_stack[ee] != TYPE_SYM)) {
            g_env_stack[ee] = relocate(g_env_stack[ee]);
        }
        ee -= 2;
    }

    relocate_symtab(symtab);

    is_gc = false;
    // g_heap poisoning for trapping bugs
    memset(g_newheap, 0x0A, oh);
}

static void relocate_symtab(Symbol* sym)
{
    sym->binding = relocate(sym->binding);
    if (sym->left) {
        relocate_symtab(sym->left);
    }
    if (sym->right) {
        relocate_symtab(sym->right);
    }
}

static value_t relocate_list(value_t);

static value_t relocate(value_t v)
{
    switch (type_of(v)) {
    case TYPE_LIST:
        return relocate_list(v);
    default:
        return v;
    }
}

static value_t _relocate_list(value_t l)
{
    value_t v = head(l);
    value_t t = tail(l);
    value_t cell = make_cell(relocate(v));
    if (t != EMPTY_LIST) {
        tail(cell) = relocate_list(t);
    }
    return cell;
}

static value_t relocate_list(value_t l)
{
    if (l == EMPTY_LIST) {
        return l;
    }
    if (head(l) != RELOCATED_MARK) {
        tail(l) = _relocate_list(l);
        head(l) = RELOCATED_MARK;
    }
    return tail(l);
}

static void* halloc(size_t s)
{
    if (g_curheap + s >= g_lim) {
        gc();
    }
    memory_t h = g_curheap;
    g_curheap += s;
    return (void*)h;
}
