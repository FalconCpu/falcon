#include <stdio.h>
#include "fpl.h"
#include "AST.h"
#include "types.h"


// ============================================================================
//                          CONSTRUCTOR
// ============================================================================

AST new_AST_index(Location location,  AST lhs, AST rhs) {
    AST_index ret = new(struct AST_index);
    ret->location = location;
    ret->kind     = AST_INDEX;
    ret->lhs      = lhs;
    ret->rhs      = rhs;
    return as_AST(ret);
}

// ============================================================================
//                          CAST
// ============================================================================

AST_index as_index(AST this) {
    assert(this->kind==AST_INDEX);
    return (AST_index) this;
}


// ============================================================================
//                          PRINT
// ============================================================================

void AST_index_print(AST_index this, int indent) {
    print_spaces(indent);
    printf("INDEX\n");
    AST_print(this->lhs, indent+1);
    AST_print(this->rhs, indent+1);
}

// ============================================================================
//                         typecheck
// ============================================================================

void AST_typecheck_index(AST_index this, Block scope) {
    AST_typecheck(this->lhs, scope);
    AST_typecheck_value(this->rhs, scope);

    if(this->lhs->type==type_error || this->rhs->type==type_error) {
        this->type = type_error;
        return;
    }

    check_type_compatible(type_int, this->rhs);

    if (is_type_expr(this->lhs)) {
        this->type = make_type_array(this->lhs->type, AST_get_constant_value(this->rhs));
        this->is_constructor = 1;
        return;
    }

    // Treat pointers to arrays as being the array itself
    Type type = this->lhs->type;
    if (type->kind==TYPE_POINTER)
        type = as_TypePointer(type)->base;

    if (type->kind==TYPE_ARRAY)
        this->type = as_TypeArray(type)->base;
    else if (type->kind==TYPE_STRING)
        this->type = type_char;
    else
        this->type = make_type_error(this->location, "Not an array type (%s)", this->lhs->type->name);
}
