#include <stdio.h>
#include "fpl.h"
#include "AST.h"
#include "types.h"

// ============================================================================
//                          CONSTRUCTOR
// ============================================================================

AST new_AST_funccall(Location location, AST lhs, AST_list rhs) {
    AST_funccall ret = new(struct AST_funccall);
    ret->location = location;
    ret->kind     = AST_FUNCCALL;
    ret->lhs      = lhs;
    ret->rhs      = rhs;
    return as_AST(ret);
}

// ============================================================================
//                          CAST
// ============================================================================

AST_funccall as_funccall(AST this) {
    assert(this->kind==AST_FUNCCALL);
    return (AST_funccall) this;
}


// ============================================================================
//                          PRINT
// ============================================================================

void AST_funccall_print(AST_funccall this, int indent) {
    print_spaces(indent);
    printf("FUNCCALL %s\n", this->type ? this->type->name : "");
    AST_print(this->lhs, indent+1);
    AST arg;
    foreach(arg, this->rhs)
        AST_print(arg, indent+1);
}

// ============================================================================
//                         typecheck
// ============================================================================

static void typecheck_funccall(AST_funccall this, Block scope) {
    // get the function's parameters
    struct Type_function* func_type = as_TypeFunction(this->lhs->type);
    TypeList param_types = func_type->parameter_types;
    if (this->rhs->count != param_types->count) {
        this->type = make_type_error(this->location, "Got %d arguments when expecting %d", this->rhs->count, param_types->count);
        return;
    }

    // type check each argument
    AST arg;
    foreach(arg, this->rhs) {
        AST_typecheck_value(arg, scope);
        check_type_compatible(TypeList_get(param_types,index), arg);
    }

    this->type = func_type->return_type;
}

static void typecheck_constructor(AST_funccall this, Block scope) {
    this->is_constructor = 1;

    // Get the struct we are creating
    if (this->lhs->type->kind!=TYPE_STRUCT) {
        this->type = make_type_error(this->location,"Not a struct type '%s' %x",this->lhs->type->name, this->lhs->type->kind);
        return;
    }

    struct Type_struct* struct_type = (struct Type_struct*) this->lhs->type;
    SymbolList members = struct_type->members;

    if (this->rhs->count != members->count) {
        this->type = make_type_error(this->location, "Got %d arguments when expecting %d", this->rhs->count, members->count);
        return;
    }

    AST arg;
    foreach(arg, this->rhs) {
        AST_typecheck_value(arg, scope);
        check_type_compatible(SymbolList_get(members,index)->type, arg);
    }

    this->type = (Type) struct_type;
}


void AST_typecheck_funccall(AST_funccall this, Block scope) {
    AST_typecheck(this->lhs, scope);
    if (this->lhs->type==type_error) {
        this->type = type_error;
        return;
    }

    if (is_type_expr(this->lhs)) {
        typecheck_constructor(this, scope);
        return;
    } else if (this->lhs->type->kind == TYPE_FUNCTION) {
        typecheck_funccall(this, scope);
        return;
    } else {
        this->type = make_type_error(this->lhs->location, "Not a function");
        return;
    }
}

