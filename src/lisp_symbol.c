#include "lisp.h"

value_t cons_(value_t h, value_t t)
{
    push(h);
    push(t);
    value_t cell = make_cell(UNBOUND);
    tail_(cell) = pop();
    head_(cell) = pop();
    return cell;
}

value_t cons(value_t h, value_t t)
{
    assert_type(t, LIST);
    return cons_(h, t);
}

static hash_t string_hash(const char* str)
{
    hash_t hash = 0x123456;
    for (int i = 0; str[i] != '\0'; ++i) {
        hash += (hash_t)str[i];
    }
    return (hash_t)(hash ^ 0x0A0A0A0A);
}

static Symbol* make_symbol(const char* name)
{
    Symbol* sym = malloc(sizeof(Symbol) + strlen(name));
    sym->binding = UNBOUND;
    sym->right = sym->left = NULL;
    sym->hash = string_hash(name);
    strcpy(sym->name, name);
    return sym;
}

// find symbol in an environment
static Symbol* _find_symbol(const char* name, hash_t hash, Symbol* root)
{
    if (root == NULL) {
        return NULL;
    }
    if (hash == root->hash) {
        if (!strcmp(name, root->name)) {
            return root;
        }
    }

    if (hash > root->hash) {
        return _find_symbol(name, hash, root->right);
    }
    return _find_symbol(name, hash, root->left);
}

Symbol* find_symbol(const char* name, Symbol** env)
{
    if (*env == NULL) {
        return NULL;
    }
    hash_t hash = string_hash(name);
    return _find_symbol(name, hash, *env);
}

static void _add_symbol(Symbol* sym, Symbol* root)
{
    if (sym->hash > root->hash) {
        if (root->right == NULL) {
            root->right = sym;
        } else {
            _add_symbol(sym, root->right);
        }
    } else {
        if (root->left == NULL) {
            root->left = sym;
        } else {
            _add_symbol(sym, root->left);
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

value_t symbol(const char* name, Symbol** env)
{
    return tagptr(_symbol(name, env), TAG_SYM);
}
