#include "lisp.h"

/**
 * @brief 字符串 hash, 随便乱写的，能达到相同字符 hash 值相同即可
 */
static hash_t string_hash(const char* str)
{
    hash_t hash = 0x123456;
    for (int i = 0; str[i] != '\0'; ++i) {
        hash += (hash_t)str[i];
    }
    return (hash_t)(hash ^ 0x0A0A0A0A);
}

/**
 * @brief 创建一个简单的符号，并计算其 Hash 值
 * @note symbol 用的是 C 的堆，故而无需被 gc 控制！
 */
static Symbol* make_symbol(const char* name)
{
    Symbol* sym = malloc(sizeof(Symbol) + strlen(name));
    sym->binding = UNBOUND;
    sym->right = sym->left = NULL;
    sym->hash = string_hash(name);
    strcpy(sym->name, name);
    return sym;
}

#if 0
static Symbol** symbol_lookup(const char* name, Symbol** env)
{
    if (*env == NULL) {
        return env;
    }
    hash_t hash = string_hash(name);
    while (*env != NULL) {
        if (hash == (*env)->hash && strcmp(name, (*env)->name)) {
            return env;
        }

        if (hash < (*env)->hash) {
            env = &(*env)->left;
        } else {
            env = &(*env)->right;
        }
    }
    return env;
}

Symbol* _symbol(const char* name, Symbol** env)
{
    Symbol** t = symbol_lookup(name, env);
    if (*t == NULL) {
        *t = make_symbol(name);
    }
    return *t;
}
#else
static Symbol* _find_symbol(const char* name, hash_t hash, Symbol* env)
{
    if (env == NULL) {
        return NULL;
    }
    if (hash == env->hash) {
        if (!strcmp(name, env->name)) {
            return env;
        }
    }

    if (hash > env->hash) {
        return _find_symbol(name, hash, env->right);
    }
    return _find_symbol(name, hash, env->left);
}

Symbol* find_symbol(const char* name, Symbol** env)
{
    if (*env == NULL) {
        return NULL;
    }
    hash_t hash = string_hash(name);
    return _find_symbol(name, hash, *env);
}

static void _add_symbol(Symbol* sym, Symbol* env)
{
    if (sym->hash > env->hash) {
        if (env->right == NULL) {
            env->right = sym;
        } else {
            _add_symbol(sym, env->right);
        }
    } else {
        if (env->left == NULL) {
            env->left = sym;
        } else {
            _add_symbol(sym, env->left);
        }
    }
    return;
}

static Symbol* add_symbol(const char* name, Symbol** env)
{
    Symbol* s = make_symbol(name);
    if (*env == NULL) {
        *env = s;
    } else {
        _add_symbol(s, *env);
    }
    return s;
}

Symbol* _symbol(const char* name, Symbol** env)
{
    Symbol* t = find_symbol(name, env);
    return t != NULL ? t : add_symbol(name, env);
}
#endif

value_t symbol(const char* name, Symbol** env)
{
    return tagptr(_symbol(name, env), TAG_SYM);
}
