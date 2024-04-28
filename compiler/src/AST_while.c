#include <stdio.h>
#include "fpl.h"
#include "AST.h"


// ============================================================================
//                          CONSTRUCTOR
// ============================================================================

AST new_AST_while(Location location, AST condition, Block body) {
    AST_while ret = new(struct AST_while);
    ret->location = location;
    ret->kind     = AST_WHILE;
    ret->condition= condition;
    ret->body     = body;
    return as_AST(ret);
}

// ============================================================================
//                          CAST
// ============================================================================

AST_while as_while(AST this) {
    assert(this->kind==AST_WHILE);
    return (AST_while) this;
}


// ============================================================================
//                          PRINT
// ============================================================================

void AST_while_print(AST_while this, int indent) {
    print_spaces(indent);
    printf("WHILE\n");
    AST_print(this->condition, indent+1);
    block_print(this->body, indent+1);
}

// ============================================================================
//                         typecheck
// ============================================================================

void AST_typecheck_while(AST_while this, Block scope) {
    AST_typecheck_value(this->condition, scope);
    check_type_compatible(type_bool, this->condition);
    block_typecheck(this->body);
}


