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
    SymbolList params = struct_type->params;

    if (this->rhs->count != params->count) {
        this->type = make_type_error(this->location, "Got %d arguments when expecting %d", this->rhs->count, params->count);
        return;
    }

    AST arg;
    foreach(arg, this->rhs) {
        AST_typecheck_value(arg, scope);
        check_type_compatible(SymbolList_get(params,index)->type, arg);
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

// ---------------------------------------------------------------------------------
//                           code_gen
// ---------------------------------------------------------------------------------

Symbol code_gen_funccall(Function func, AST_funccall this) {
    if (this->is_constructor)
        fatal("Internal error - code_gen_funccall called in a scalar context");

    // evaluate all the arguments
    SymbolList arg_list = new_SymbolList();
    AST arg_ast;
    foreach(arg_ast, this->rhs)
        SymbolList_add(arg_list, code_gen(func, arg_ast));

    if (arg_list->count>7)
        fatal("Compiler currently only supports functions with <=7 parameters");


    // member functions have an implicit this parameter. 
    // if called using a.func() notation then get the argument for 'this' from the member function
    // otherwise we pass our own 'this' symbol
    Symbol implicit_this = func->this_sym;  

    // Find the function to call
    Symbol func_addr;
    if (this->lhs->kind==AST_MEMBER && as_member(this->lhs)->symbol->kind==SYM_FUNCTION) {
        implicit_this = code_gen(func, as_member(this->lhs)->lhs);
        func_addr = as_member(this->lhs)->symbol;
    } else if (this->lhs->kind==AST_SYMBOL && as_symbol(this->lhs)->symbol->kind==SYM_FUNCTION)
        func_addr = as_symbol(this->lhs)->symbol;
    else {
        func_addr = code_gen(func, this->lhs);
    }

    int param_no = 1;

    // look to see if the function has an implicit this parameter
    if (func_addr->kind==SYM_FUNCTION && func_addr->value.function->this_sym)
        add_instr(func, new_Instr(INSTR_MOV, 0, SymbolList_get(func->all_vars, param_no++), implicit_this, 0));
    
    // and pass the rest of the arguments
    Symbol arg;
    foreach_symbol(arg, arg_list)
        add_instr(func, new_Instr(INSTR_MOV, 0, SymbolList_get(func->all_vars, param_no++), arg, 0));
    add_instr(func, new_Instr(INSTR_CALL, 0, 0, func_addr, 0));

    Symbol ret = 0;
    if (this->type != type_void) {
        ret = new_tempvar(func, this->type);
        add_instr(func, new_Instr(INSTR_MOV, 0, ret, SymbolList_get(func->all_vars, 8), 0));
    }
    return ret;
}


void code_gen_store_at_funccall(Function func, AST_funccall this, Symbol dest) {
    if (! this->is_constructor)
        TODO("Function calls with aggregate return type");

    if (this->type->kind==TYPE_STRUCT) {
        Type_struct this_struct = as_TypeStruct(this->type);
        AST arg;
        foreach(arg, this->rhs) {
            Symbol param = SymbolList_get(this_struct->params,index);
            if (is_scalar_type(param->type)) {
                Symbol a = code_gen(func, arg);
                add_instr(func, new_Instr(INSTR_STORE, get_sizeof(param->type), a, dest, param));
            } else {
                TODO("Aggregate types inside struct");
            }

        }
    }
}