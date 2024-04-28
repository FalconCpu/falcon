#include <stdio.h>
#include "fpl.h"
#include "AST.h"


// ============================================================================
//                          CONSTRUCTOR
// ============================================================================

AST new_AST_intlit(Location location, Type type, int value) {
    AST_intlit ret = new(struct AST_intlit);
    ret->location = location;
    ret->kind     = AST_INTLIT;
    ret->type     = type;
    ret->value    = value;
    return as_AST(ret);
}

// ============================================================================
//                          CAST
// ============================================================================

AST_intlit as_intlit(AST this) {
    assert(this->kind==AST_INTLIT);
    return (AST_intlit) this;
}

// ============================================================================
//                         typecheck
// ============================================================================

void AST_typecheck_intlit(AST_intlit this, Block block) {
    // everything is checked at parse time - so nothing to do here
    (void) this;
    (void) block;
}

// ============================================================================
//                          PRINT
// ============================================================================

void AST_intlit_print(AST_intlit this, int indent) {
    print_spaces(indent);
    printf("INTLIT %d %s\n", this->value, this->type->name);
}

