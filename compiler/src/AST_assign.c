#include <stdio.h>
#include "fpl.h"
#include "AST.h"


// ============================================================================
//                          CONSTRUCTOR
// ============================================================================

AST new_AST_assign(Location location, AST lhs, AST rhs) {
    AST_assign ret = new(struct AST_assign);
    ret->location = location;
    ret->kind     = AST_ASSIGN;
    ret->lhs      = lhs;
    ret->rhs      = rhs;
    return as_AST(ret);
}

// ============================================================================
//                          CAST
// ============================================================================

AST_assign as_assign(AST this) {
    assert(this->kind==AST_ASSIGN);
    return (AST_assign) this;
}


// ============================================================================
//                          PRINT
// ============================================================================

void AST_assign_print(AST_assign this, int indent) {
    print_spaces(indent);
    printf("ASSIGN\n");
    AST_print(this->lhs, indent+1);
    AST_print(this->rhs, indent+1);
}

// ============================================================================
//                         typecheck
// ============================================================================

void AST_typecheck_assign(AST_assign this, Block scope) {
    AST_typecheck_value(this->lhs, scope);
    AST_typecheck_value(this->rhs, scope);
    check_lvalue(this->lhs);
    check_type_compatible(this->lhs->type, this->rhs);
}


// ============================================================================
//                           code_gen
// ============================================================================

Symbol code_gen_assign(Function func, AST_assign this) {
    if (is_scalar_type(this->rhs->type)) {
        Symbol rhs = code_gen(func, this->rhs);
        code_gen_lvalue(func, this->lhs, rhs);
    } else {
        Symbol lhs = code_gen_aggregate_lhs(func, this->lhs);
        code_gen_aggregate_rhs(func, this->rhs, lhs);
    }
    return 0;
}
