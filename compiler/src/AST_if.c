#include <stdio.h>
#include "fpl.h"
#include "AST.h"


// ============================================================================
//                          CONSTRUCTOR
// ============================================================================

AST new_AST_if(Location location, Block clauses) {
    AST_if ret = new(struct AST_if);
    ret->location = location;
    ret->kind     = AST_IF;
    ret->clauses  = clauses;
    return as_AST(ret);
}

// ============================================================================
//                          CAST
// ============================================================================

AST_if as_if(AST this) {
    assert(this->kind==AST_IF);
    return (AST_if) this;
}


// ============================================================================
//                          PRINT
// ============================================================================

void AST_if_print(AST_if this, int indent) {
    print_spaces(indent);
    printf("IF\n");
    AST clause;
    foreach_statement(clause, this->clauses)
        AST_print(clause, indent+1);
}


// ============================================================================
//                         typecheck
// ============================================================================

void AST_typecheck_if(AST_if this, Block scope) {
    (void) scope;
    block_typecheck(this->clauses);
}

