#include <stdio.h>
#include <string.h>
#include "fpl.h"
#include "AST.h"
#include "types.h"

// ============================================================================
//                          CONSTRUCTOR
// ============================================================================

AST new_AST_member(Location location,  AST lhs, String name, int null_coalese) {
    AST_member ret = new(struct AST_member);
    ret->location = location;
    ret->kind     = AST_MEMBER;
    ret->lhs      = lhs;
    ret->name     = name;
    ret->null_coalese = null_coalese;
    return as_AST(ret);
}

// ============================================================================
//                          CAST
// ============================================================================

AST_member as_member(AST this) {
    assert(this->kind==AST_MEMBER);
    return (AST_member) this;
}


// ============================================================================
//                          PRINT
// ============================================================================

void AST_member_print(AST_member this, int indent) {
    print_spaces(indent);
    printf("MEMBER %s %s\n", this->name, this->type? this->type->name : "");
    AST_print(this->lhs, indent+1);
}

// ============================================================================
//                         typecheck
// ============================================================================

void AST_typecheck_member(AST_member this, Block scope) {
    AST_typecheck_value(this->lhs, scope);

    if (this->lhs->type == type_error) {
        this->type = type_error;
        return;
    }

    // special case (for now). Can be applied to string or array to get size
    if ((this->lhs->type->kind==TYPE_STRING || this->lhs->type->kind==TYPE_ARRAY) && !strcmp(this->name,"size")) {
        this->symbol = new_Symbol(0, SYM_VARIABLE, "size", type_int, 0);
        this->symbol->offset = -4;
        this->type = type_int;
        return;
    }

    struct Type_struct *struct_type=0;
    int doing_null_coalese = 0;

    if (this->lhs->type->kind==TYPE_STRUCT) {
        struct_type = (struct Type_struct *) this->lhs->type;
        if (this->null_coalese)
            warn(this->location, "Redundant use of null coalesing operator (could just use . )");

    } else if (this->lhs->type->kind==TYPE_POINTER) {
        struct Type_pointer* pointer = as_TypePointer(this->lhs->type);
        if (pointer->nullable && !this->null_coalese) {
            this->type = make_type_error(this->location,"Accessing field through nullish pointer (consider using ?. )");
            return;
        } else if (!pointer->nullable && this->null_coalese)
            warn(this->location, "Redundant use of null coalesing operator (could just use . )");
        else if (pointer->nullable && this->null_coalese)
            doing_null_coalese = 1;

        if (pointer->base->kind==TYPE_STRUCT)
            struct_type = (struct Type_struct *) pointer->base;
    }

    if (struct_type==0) {
        this->type = make_type_error(this->location,"Not a struct type (%s)", this->lhs->type->name);
        return;
    }

    this->symbol = SymbolList_find(struct_type->members, this->name);

    if (this->symbol==0)
        this->type = make_type_error(this->location,"Struct '%s' has no member named '%s'", struct_type->name, this->name);
    else
        this->type = this->symbol->type;

    if (doing_null_coalese && this->type->kind==TYPE_POINTER) {
        struct Type_pointer* type_p = (struct Type_pointer*) this->type;
        this->type = make_type_pointer(type_p->base, 1);
    }
}

// ============================================================================
//                         code_gen
// ============================================================================

Symbol code_gen_member(Function func, AST_member this) {
    Symbol lhs = is_scalar_type(this->lhs->type) ? code_gen(func, this->lhs) : code_gen_aggregate_lhs(func, this->lhs);
    Symbol ret = new_tempvar(func, this->type);

    if (!is_scalar_type(this->type))
        fatal("code_gen called with non scalar type");

    int size = get_sizeof(this->type);
    add_instr(func, new_Instr(INSTR_LOAD, size, ret, lhs, make_constant_symbol(func, this->symbol->offset)));
    // } else {
    //     add_instr(func, new_Instr(INSTR_ALU, ALU_ADD_I, ret, lhs, make_constant_symbol(func, this->symbol->offset)));
    // }
    return ret;
}


// ============================================================================
//                           code_gen_lvalue
// ============================================================================

void code_gen_lvalue_member(Function func, AST_member this, Symbol value) {
    Symbol lhs = is_scalar_type(this->lhs->type) ? code_gen(func, this->lhs) : code_gen_aggregate_lhs(func, this->lhs);

    int size = get_sizeof(this->type);
    add_instr(func, new_Instr(INSTR_STORE, size, value, lhs, make_constant_symbol(func, this->symbol->offset)));
}