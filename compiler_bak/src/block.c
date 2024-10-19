#include "stdio.h"
#include "fpl.h"

// ===========================================================================
//                    CONSTRUCTOR
// ===========================================================================

Block new_block(Block parent) {
    Block ret = new(struct Block);
    ret->parent = parent;
    ret->alloc_statements = 16;
    ret->statements = new_array(AST, ret->alloc_statements);
    ret->symbols = new_SymbolList();
    return ret;
}

// ===========================================================================
//                    block_add_statement
// ===========================================================================

void  block_add_statement(Block this, AST statement) {
    if (statement==0)
        return;

    if (this->num_statements==this->alloc_statements) {
        this->alloc_statements *= 2;
        resize_array(this->statements, AST, this->alloc_statements);
    }
    this->statements[this->num_statements++] = statement;
}

// ===========================================================================
//                    block_get_statement
// ===========================================================================

AST  block_get_statement(Block this, int index) {
    assert(this);
    assert(index>=0);
    assert(index<=this->num_statements);
    if (index==this->num_statements)
        return 0;
    else
        return this->statements[index];

}

// ===========================================================================
//                  block_add_symbol
// ===========================================================================
// Add a symbol to a block
// Reports an error in the case of duplicate names

void block_add_symbol(Block block, Symbol sym) {
    if (sym==0)
        return;     // Safely do nothing if passed null symbol
    if (block==0)
        fatal("Internal error: Null pointer in ASTblock_add_symbol");

    Symbol duplicate = SymbolList_find(block->symbols, sym->name);
    if (duplicate)
        error(sym->location, "Duplicate symbol '%s', first at %s", sym->name, location_toString(duplicate->location));

    SymbolList_add(block->symbols, sym);
}

// ===========================================================================
//                ASTblock_find_symbol
// ===========================================================================
// Look up a symbol (hierarchcally)
// Returns null if symbol not found

extern SymbolList builtin_types;

Symbol block_find_symbol(Block block, String name) {
    Symbol sym = SymbolList_find(builtin_types, name);
    if (sym!=0)
        return sym;

    while(block) {
        Symbol s = SymbolList_find(block->symbols, name);
        if (s) {
            return s;
        }
        block = block->parent;
    }
    return 0;
}

// ===========================================================================
//                    block_print
// ===========================================================================

void  block_print(Block this, int indent) {
    if (this==0)    
        return;

    if (this->symbols->count!=0) {
        print_spaces(indent);
        printf("Symbol table [");
        Symbol s;
        foreach_symbol(s, this->symbols)
            printf("%s ",Symbol_toText(s));
        printf("]\n");
    }

    AST stmt;
    foreach_statement(stmt, this)
        AST_print(stmt,indent);
}


// ===========================================================================
//                         ASTblock_find_enclosing_function
// ===========================================================================
// Look up the scope hierarchy to find the enclosing function definition
// returns null if the not within a function
Function block_find_enclosing_function(Block this) {
    while(this) {
        if (this->enclosing_statement!=0 && this->enclosing_statement->kind==AST_FUNCTION)
            return as_function(this->enclosing_statement);
        this=this->parent;

    }
    return 0;
}

// ===========================================================================
//                           code_gen
// ===========================================================================

Symbol code_gen_block(Function func, Block this) {
    AST stmt;
    foreach_statement(stmt, this)
        if (stmt->kind!=AST_FUNCTION)
            code_gen(func, stmt);
    return 0;
}