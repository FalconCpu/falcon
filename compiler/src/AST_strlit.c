#include <stdio.h>
#include "fpl.h"
#include "AST.h"


// ============================================================================
//                          CONSTRUCTOR
// ============================================================================

AST new_AST_strlit(Location location, Type type, String value) {
    AST_strlit ret = new(struct AST_strlit);
    ret->location = location;
    ret->kind     = AST_STRLIT;
    ret->type     = type;
    ret->value    = value;
    return as_AST(ret);
}

// ============================================================================
//                          CAST
// ============================================================================

AST_strlit as_strlit(AST this) {
    assert(this->kind==AST_STRLIT);
    return (AST_strlit) this;
}

// ============================================================================
//                         typecheck
// ============================================================================

void AST_typecheck_strlit(AST_strlit this, Block block) {
    // everything is checked at parse time - so nothing to do here
    (void) this;
    (void) block;
}

// ============================================================================
//                          PRINT
// ============================================================================

void AST_strlit_print(AST_strlit this, int indent) {
    print_spaces(indent);
    printf("STRLIT %s %s\n", this->value, this->type->name);
}

