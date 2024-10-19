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

// ---------------------------------------------------------------------------------
//                           get_array_bounds
// ---------------------------------------------------------------------------------

static Symbol get_array_bounds(Function func, Symbol lhs) {
    // We get the array bounds from the type (if known), otherwise from a length field which
    // is stored just before the array itself in memory.
    Symbol bounds;

    Type type = lhs->type;
    if (type->kind==TYPE_POINTER)
        type = as_TypePointer(type)->base;

    if (type->kind==TYPE_STRING) {
        bounds = new_tempvar(func, type_int);
        add_instr(func, new_Instr(INSTR_LOAD, 4, bounds, lhs, stdlib.size));
    } else {
        struct Type_array* array_type = as_TypeArray(type);
        if (array_type->size!=0)
            bounds = make_constant_symbol(func, array_type->size);
        else {
            bounds = new_tempvar(func, type_int);
            add_instr(func, new_Instr(INSTR_LOAD, 4, bounds, lhs, stdlib.size));
        }
    }
    return bounds;
}

// ---------------------------------------------------------------------------------
//                           calculate_address
// ---------------------------------------------------------------------------------

static Symbol calculate_address(Function func, int size, Symbol lhs, Symbol rhs) {
    Symbol mult = new_tempvar(func, type_int);
    add_instr(func, new_Instr(INSTR_ALU, ALU_MUL_I, mult, rhs, make_constant_symbol(func, size)));
    Symbol addr = new_tempvar(func, type_int);
    add_instr(func, new_Instr(INSTR_ALU, ALU_ADD_I, addr, lhs, mult));
    return addr;
}

// ---------------------------------------------------------------------------------
//                           code_gen_index
// ---------------------------------------------------------------------------------

Symbol code_gen_index(Function func, AST_index this) {
    Symbol lhs = is_scalar_type(this->lhs->type) ? code_gen(func, this->lhs) : code_gen_address_of(func, this->lhs);
    Symbol rhs = code_gen(func, this->rhs);

    // For bounds checking we emit a pseudo CHK instruction here. Later code in the
    // optimizer will try to elide the check if possible, or if not convert it into a branch
    // to the exception handler
    add_instr(func, new_Instr(INSTR_CHK, 0, 0, rhs, get_array_bounds(func,lhs)));

    int size = get_sizeof(this->type);
    Symbol addr = calculate_address(func, size, lhs, rhs);

    if (!is_scalar_type(this->type))
        fatal("code_gen_index called with non scalar type");

    Symbol ret = new_tempvar(func, this->type);
    add_instr(func, new_Instr(INSTR_LOAD, size, ret, addr, make_constant_symbol(func, 0)));
    return ret;
}


// ---------------------------------------------------------------------------------
//                           code_gen_address_of_index
// ---------------------------------------------------------------------------------

Symbol code_gen_address_of_index(Function func, AST_index this) {
    Symbol lhs = is_scalar_type(this->lhs->type) ? code_gen(func, this->lhs) : code_gen_address_of(func, this->lhs);
    Symbol rhs = code_gen(func, this->rhs);

    // For bounds checking we emit a pseudo CHK instruction here. Later code in the
    // optimizer will try to elide the check if possible, or if not convert it into a branch
    // to the exception handler
    add_instr(func, new_Instr(INSTR_CHK, 0, 0, rhs, get_array_bounds(func,lhs)));

    int size = get_sizeof(this->type);
    return calculate_address(func, size, lhs, rhs);
}



// ---------------------------------------------------------------------------------
//                           code_gen_lvalue
// ---------------------------------------------------------------------------------

void code_gen_lvalue_index(Function func, AST_index this, Symbol value) {
    Symbol lhs = is_scalar_type(this->lhs->type) ? code_gen(func, this->lhs) : code_gen_address_of(func, this->lhs);
    Symbol rhs = code_gen(func, this->rhs);

    // For bounds checking we emit a pseudo CHK instruction here. Later code in the
    // optimizer will try to elide the check if possible, or if not convert it into a branch
    // to the exception handler
    add_instr(func, new_Instr(INSTR_CHK, 0, 0, rhs, get_array_bounds(func,lhs)));

    int size = get_sizeof(this->type);
    Symbol addr = calculate_address(func, size, lhs, rhs);

    add_instr(func, new_Instr(INSTR_STORE, size, value, addr, make_constant_symbol(func, 0)));
}

// ---------------------------------------------------------------------------------
//                           code_gen_store_at_index
// ---------------------------------------------------------------------------------

void  code_gen_store_at_index(Function func, AST_index this, Symbol dest) {
    if (this->is_constructor) {
        // TODO - allow dynamically configurable sizes
        int size = get_sizeof(this->type);
        add_instr(func, new_Instr(INSTR_MOV, 0, SymbolList_get(func->all_vars,1), dest,0));
        add_instr(func, new_Instr(INSTR_MOV, 0, SymbolList_get(func->all_vars,2), make_constant_symbol(func,size),0));
        add_instr(func, new_Instr(INSTR_CALL, 0, 0, stdlib.sdlib_bzero, 0));
        Symbol num_index = make_constant_symbol(func,as_TypeArray(this->type)->size);
        Symbol num_index_reg = new_tempvar(func,type_int);
        add_instr(func, new_Instr(INSTR_MOV, 0, num_index_reg, num_index, 0));
        add_instr(func, new_Instr(INSTR_STORE, 4, num_index_reg, dest, stdlib.size));

    } else {
        Symbol lhs = is_scalar_type(this->lhs->type) ? code_gen(func, this->lhs) : code_gen_address_of(func, this->lhs);
        Symbol rhs = code_gen(func, this->rhs);
        add_instr(func, new_Instr(INSTR_CHK, 0, 0, rhs, get_array_bounds(func,lhs)));
        int size = get_sizeof(this->type);
        Symbol addr = calculate_address(func, size, lhs, rhs);
        gen_memcopy(func, dest, addr, size);
    }
}