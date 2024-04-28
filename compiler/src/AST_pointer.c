#include <stdio.h>
#include "fpl.h"
#include "AST.h"
#include "types.h"

// ============================================================================
//                          CONSTRUCTOR
// ============================================================================

AST new_AST_pointer(Location location, AST rhs) {
    AST_pointer ret = new(struct AST_pointer);
    ret->location = location;
    ret->kind     = AST_POINTER;
    ret->rhs      = rhs;
    return as_AST(ret);
}

// ============================================================================
//                          CAST
// ============================================================================

AST_pointer as_pointer(AST this) {
    assert(this->kind==AST_POINTER);
    return (AST_pointer) this;
}

// ============================================================================
//                          PRINT
// ============================================================================

void AST_pointer_print(AST_pointer this, int indent) {
    print_spaces(indent);
    printf("POINTER %s\n", this->type->name);
    AST_print(this->rhs, indent+1);
}

// ============================================================================
//                         typecheck
// ============================================================================

void AST_typecheck_pointer(AST_pointer this, Block scope) {
    AST_typecheck_value(this->rhs, scope);

    if (this->rhs->type==type_error) {
        this->type = type_error;
        return;
    } else if (this->rhs->type->kind != TYPE_POINTER) {
        this->type = make_type_error(this->location, "Not a pointer type (%s)", this->rhs->type->name);
        return;
    } else {
        this->type = as_TypePointer(this->rhs->type)->base;
        return;
    }
}