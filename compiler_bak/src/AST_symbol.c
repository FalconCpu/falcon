#include <stdio.h>
#include "fpl.h"
#include "AST.h"
#include "instr.h"


// ============================================================================
//                          CONSTRUCTOR
// ============================================================================

AST new_AST_symbol(Location location, String name) {
    AST_symbol ret = new(struct AST_symbol);
    ret->location = location;
    ret->kind     = AST_SYMBOL;
    ret->name     = name;
    return as_AST(ret);
}

AST new_ASTnode_id_from_symbol(Symbol sym) {
    AST_symbol ret = new(struct AST_symbol);
    ret->location = sym->location;
    ret->kind = AST_SYMBOL;
    ret->name = sym->name;
    ret->type = sym->type;
    ret->symbol = sym;
    return as_AST(ret);
}


// ============================================================================
//                          CAST
// ============================================================================

AST_symbol as_symbol(AST this) {
    assert(this->kind==AST_SYMBOL);
    return (AST_symbol) this;
}


// ============================================================================
//                          PRINT
// ============================================================================

void AST_symbol_print(AST_symbol this, int indent) {
    print_spaces(indent);
    printf("SYMBOL %s %s\n", this->name, this->type ? this->type->name : "");
}

// ============================================================================
//                         typeCheck
// ============================================================================
// Resolve a reference to a symbol .

void AST_typecheck_symbol(AST_symbol this, Block scope) {
    this->symbol = block_find_symbol(scope, this->name);

    if (this->symbol==0) {
        this->symbol = new_Symbol(this->location, SYM_VARIABLE, this->name,
                make_type_error(this->location, "Undefined symbol '%s'", this->name), 0);
        block_add_symbol(scope, this->symbol);
    }

    this->type = this->symbol->type;
}

// ---------------------------------------------------------------------------------
//                           code_gen
// ---------------------------------------------------------------------------------

Symbol code_gen_symbol(Function func, AST_symbol this) {

    switch(this->symbol->kind) {
        case SYM_GLOBALVAR: {
            Symbol ret = new_tempvar(func, this->type);
            add_instr(func, new_Instr(INSTR_LOAD, get_sizeof(this->type), ret, SymbolList_get(func->all_vars,REG_GLOBALS), this->symbol));
            return ret;
        }

        case SYM_CONSTANT:
            if (this->type==type_null || this->type==type_bool || this->type==type_char || this->type==type_int)
                return make_constant_symbol(func, this->symbol->value.int_val);
            else
                return this->symbol;

        case SYM_VARIABLE:
            return get_codegen_symbol(func, this->symbol);

        case SYM_FIELD: {
            Symbol ret = new_tempvar(func, this->type);
            add_instr(func, new_Instr(INSTR_LOAD, get_sizeof(this->type), ret, func->this_sym, this->symbol));
            return ret;
        }

        default:
            fatal("Got kind %x in code_gen_symbol", this->symbol->kind);
    }
}

// ---------------------------------------------------------------------------------
//                           code_gen_lvalue
// ---------------------------------------------------------------------------------

void code_gen_lvalue_symbol(Function func, AST_symbol this, Symbol value) {
    switch(this->symbol->kind) {
        case SYM_GLOBALVAR:
            add_instr(func, new_Instr(INSTR_STORE, get_sizeof(this->type), value, SymbolList_get(func->all_vars,REG_GLOBALS), this->symbol));
            break;

        case SYM_VARIABLE: {
            Symbol sym = get_codegen_symbol(func, this->symbol);
            add_instr(func, new_Instr(INSTR_MOV, 0, sym, value, 0));
            break;
        }

        case SYM_FIELD:
            add_instr(func, new_Instr(INSTR_STORE, get_sizeof(this->type), value, func->this_sym, this->symbol));
            break;

        default:
            fatal("Got kind %x in code_gen_symbol", this->symbol->kind);
    }
}

// ---------------------------------------------------------------------------------
//                         code_gen_address_of
// ---------------------------------------------------------------------------------
// Run code generation on an AST 

Symbol  code_gen_address_of_symbol(Function func, AST_symbol this) {
    Symbol s = get_codegen_symbol(func, this->symbol);
    switch(s->kind) {
        case SYM_AGGREGATE: {
            // We have an variable on our local stack
            Symbol ret = new_tempvar(func, make_type_pointer(this->type,0));
            Symbol sp = SymbolList_get(func->all_vars,REG_SP);
            Symbol offset = make_constant_symbol(func, s->offset);
            add_instr(func, new_Instr(INSTR_ALU, ALU_ADD_I, ret, sp, offset));
            return ret;
        }

        case SYM_VARIABLE:
            // We have a variable that was passed into the function
            return s;

        case SYM_GLOBALVAR: {
            Symbol ret = new_tempvar(func, make_type_pointer(this->type,0));
            Symbol gp = SymbolList_get(func->all_vars,REG_GLOBALS);
            add_instr(func, new_Instr(INSTR_ALU, ALU_ADD_I, ret, gp, this->symbol));
            return ret;
        }


        default:
            fatal("Got kind %x in code_gen_address_of_id", s->kind);
    }
}

// ---------------------------------------------------------------------------------
//                         code_gen_store_at
// ---------------------------------------------------------------------------------
// Run code generation on an AST - storing the result at an address

void  code_gen_store_at_symbol(Function func, AST_symbol this, Symbol dest) {
    Symbol s = get_codegen_symbol(func, this->symbol);

    Symbol addr;
    switch(s->kind) {
        case SYM_AGGREGATE: {
            // We have an variable on our local stack
            addr = new_tempvar(func, make_type_pointer(this->type,0));
            Symbol sp = SymbolList_get(func->all_vars,REG_SP);
            Symbol offset = make_constant_symbol(func, s->offset);
            add_instr(func, new_Instr(INSTR_ALU, ALU_ADD_I, addr , sp, offset));
            break;
        }

        case SYM_VARIABLE:
            // We have a variable that was passed into the function
            addr = s;
            break;

        case SYM_GLOBALVAR: {
            addr = new_tempvar(func, make_type_pointer(this->type,0));
            Symbol gp = SymbolList_get(func->all_vars,REG_GLOBALS);
            add_instr(func, new_Instr(INSTR_ALU, ALU_ADD_I, addr, gp, this->symbol));
            break;
        }

        default:
            fatal("Got kind %x in code_gen_address_of_id", s->kind);
    }

    gen_memcopy(func, dest, addr,  get_sizeof(this->type));
}

