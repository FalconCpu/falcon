#include <stdio.h>
#include "fpl.h"
#include "AST.h"
#include "types.h"


// ============================================================================
//                          CONSTRUCTOR
// ============================================================================

AST new_AST_new(Location location, AST rhs) {
    AST_new ret = new(struct AST_new);
    ret->location = location;
    ret->kind     = AST_NEW;
    ret->rhs      = rhs;
    return as_AST(ret);
}

// ============================================================================
//                          CAST
// ============================================================================

AST_new as_new(AST this) {
    assert(this->kind==AST_NEW);
    return (AST_new) this;
}


// ============================================================================
//                          PRINT
// ============================================================================

void AST_new_print(AST_new this, int indent) {
    print_spaces(indent);
    printf("NEW %s\n", this->type?this->type->name : "");
    AST_print(this->rhs, indent+1);
}

// ============================================================================
//                         typeCheck
// ============================================================================
// Typecheck a unary operator

void AST_typecheck_new(AST_new this, Block scope) {
    AST_typecheck_value(this->rhs, scope);

    if (this->rhs->type==type_error) {
        this->type = type_error;
        return;
    }

    if (this->rhs->type->kind != TYPE_STRUCT) {
        this->type = make_type_error(this->location,"New must be called on a struct value");
        return;
    }

    this->type = make_type_pointer(this->rhs->type, 0);
}

// ---------------------------------------------------------------------------------
//                           code_gen
// ---------------------------------------------------------------------------------

Symbol code_gen_new(Function func, AST_new this) {
    Type_struct ts = as_TypeStruct(this->rhs->type);
    Symbol ret = new_tempvar(func, this->type);
    add_instr(func, new_Instr(INSTR_LEA, 0, SymbolList_get(func->all_vars,1), ts->symbol, 0)); 
    add_instr(func, new_Instr(INSTR_CALL, 0, 0, stdlib.stdlib_new, 0));
    add_instr(func, new_Instr(INSTR_MOV, 0, ret, SymbolList_get(func->all_vars,REG_RESULT), 0)); 
    code_gen_store_at(func, this->rhs, ret);
    return ret;
}