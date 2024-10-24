#include <stdio.h>
#include "fpl.h"

typedef struct Symbol_local* Symbol_local;
typedef struct Symbol_global* Symbol_global;

// ===========================================================
//                    Symbol_local
// ===========================================================

struct Symbol_local {
    int    kind;
    String name;
    Type   type;
};

Symbol new_symbol_local(String name, Type type, int mutable) {
    Symbol_local ret = new(struct Symbol_local);
    ret->kind = SYMBOL_LOCAL;
    ret->name = name;
    ret->type = type;
    return (Symbol) ret;
}

// ===========================================================
//                    new_symbol_global
// ===========================================================

struct Symbol_global {
    int    kind;
    String name;
    Type   type;
};

Symbol new_symbol_global(String name, Type type, int mutable) {
    Symbol_global ret = new(struct Symbol_global);
    ret->kind = SYMBOL_LOCAL;
    ret->name = name;
    ret->type = type;
    return (Symbol) ret;
}

