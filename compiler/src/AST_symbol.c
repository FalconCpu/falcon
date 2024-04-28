#include <stdio.h>
#include "fpl.h"
#include "AST.h"


// ============================================================================
//                          CONSTRUCTOR
// ============================================================================

AST new_AST_symbol(Location location, String name) {
    AST_symbol ret = new(struct AST_symbol);
    ret->location = location;
    ret->kind     = AST_SYMBOL;
    ret->name     = name;
    return as_AST(ret);
}

// ============================================================================
//                          CAST
// ============================================================================

AST_symbol as_symbol(AST this) {
    assert(this->kind==AST_SYMBOL);
    return (AST_symbol) this;
}


// ============================================================================
//                          PRINT
// ============================================================================

void AST_symbol_print(AST_symbol this, int indent) {
    print_spaces(indent);
    printf("SYMBOL %s %s\n", this->name, this->type ? this->type->name : "");
}

// ============================================================================
//                         typeCheck
// ============================================================================
// Resolve a reference to a symbol .

void AST_typecheck_symbol(AST_symbol this, Block scope) {
    this->symbol = block_find_symbol(scope, this->name);

    if (this->symbol==0) {
        this->symbol = new_Symbol(this->location, SYM_VARIABLE, this->name,
                make_type_error(this->location, "Undefined symbol '%s'", this->name), 0);
        block_add_symbol(scope, this->symbol);
    }

    this->type = this->symbol->type;
}


