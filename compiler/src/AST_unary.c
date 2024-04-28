#include <stdio.h>
#include "fpl.h"
#include "AST.h"


// ============================================================================
//                          CONSTRUCTOR
// ============================================================================

AST new_AST_unary(Location location, TokenKind op, AST rhs) {
    AST_unary ret = new(struct AST_unary);
    ret->location = location;
    ret->kind     = AST_UNARY;
    ret->op       = op;
    ret->rhs      = rhs;
    return as_AST(ret);
}

// ============================================================================
//                          CAST
// ============================================================================

AST_unary as_unary(AST this) {
    assert(this->kind==AST_UNARY);
    return (AST_unary) this;
}


// ============================================================================
//                          PRINT
// ============================================================================

void AST_unary_print(AST_unary this, int indent) {
    print_spaces(indent);
    printf("UNARY %s\n", token_kind_names[this->op]);
    AST_print(this->rhs, indent+1);
}

// ============================================================================
//                         typeCheck
// ============================================================================
// Typecheck a unary operator

void AST_typecheck_unary(AST_unary this, Block scope) {
    AST_typecheck_value(this->rhs, scope);

    if (this->rhs->type==type_error) {
        this->type = type_error;
        return;
    }

    // TODO - handle type promotions from char/short to int

    if (this->op==TOK_MINUS && this->rhs->type==type_int) {
        this->type = type_int;
    } else if (this->op==TOK_MINUS && this->rhs->type==type_real) {
        this->type = type_real;
    } else if (this->op==TOK_AMPERSAND) {
        this->type = make_type_pointer(this->rhs->type, 0);
    } else {
        this->type = make_type_error(this->location, "No operation defined for %s %s", token_kind_names[this->op], this->rhs->type);
    }
}
