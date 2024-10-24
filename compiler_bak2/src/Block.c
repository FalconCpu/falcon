#include <stdio.h>
#include "fpl.h"

// ====================================================
//                    safe cast
// ====================================================

Block as_block(AST this) {
    if (this->kind==AST_TOP || this->kind==AST_FUNCTION)
        return (Block) this;
    fatal("Internal error: Attempt to cast %2x to block", this->kind);
}

// ====================================================
//                    block_add_statement
// ====================================================

void block_add_statement(Block b, AST statement) {
    if (statement==0)
        return;
    if (b->alloc_statements==0) {
        b->alloc_statements = 8;
        b->statements = new_array(AST, b->alloc_statements);
    } else if (b->num_statements==b->alloc_statements) {
        b->alloc_statements *= 2;
        resize_array(b->statements, AST, b->alloc_statements);
    }
    b->statements[b->num_statements++] = statement;
}

// =====================================================
//                    block_add_symbol
// ====================================================

void block_add_symbol(Block b, Symbol s) {
    if (s==0)
        return;
    Symbol dup;
    foreach_symbol(b,dup)
        if (dup->name==s->name)
            error(get_location(s),"Duplicate symbol %s", s->name);
    if (b->alloc_symbols==0) {
        b->alloc_symbols = 8;
        b->symbols = new_array(Symbol, b->alloc_symbols);
    } else if (b->num_symbols==b->alloc_symbols) {
        b->alloc_symbols *= 2;
        resize_array(b->symbols, Symbol, b->alloc_symbols);
    }
    b->symbols[b->num_symbols++] = s;
}

// =====================================================
//                    block_add_type
// ====================================================

void block_add_type(Block b, Type t) {
    if (t==0)
        return;
    Type dup;
    foreach_type(b,dup)
        if (dup->name==t->name)
            error(get_location(t),"Duplicate type %s", t->name);
    if (b->alloc_types==0) {
        b->alloc_types = 8;
        b->types = new_array(Type, b->alloc_types);
    } else if (b->num_types==b->alloc_types) {
        b->alloc_types *= 2;
        resize_array(b->types, Type, b->alloc_types);
    }
    b->types[b->num_types++] = t;
}

// =====================================================
//                    block_lookup_symbol
// ====================================================

Symbol block_lookup_symbol(Block b, String name) {
    Symbol s;
    while(b) {
        foreach_symbol(b,s)
            if (s->name == name)
                return s;
        b = b->parent;
    }
    return 0;
}

// =====================================================
//                    block_lookup_type
// ====================================================

Type block_lookup_type(Block b, String name) {
    Type t;
    while(b) {
        foreach_type(b,t)
            if (t->name == name)
                return t;
        b = b->parent;
    }
    return 0;
}
