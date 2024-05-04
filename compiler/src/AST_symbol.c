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
    if (this->symbol->kind==SYM_GLOBALVAR) {
        Symbol ret = new_tempvar(func, this->type);
        add_instr(func, new_Instr(INSTR_LOAD, get_sizeof(this->type), ret, SymbolList_get(func->all_vars,REG_GLOBALS), this->symbol));
        return ret;
    } else
        return get_codegen_symbol(func, this->symbol);
}

// ---------------------------------------------------------------------------------
//                           code_gen_lvalue
// ---------------------------------------------------------------------------------

void code_gen_lvalue_symbol(Function func, AST_symbol this, Symbol value) {
    if (this->symbol->kind==SYM_GLOBALVAR) {
        add_instr(func, new_Instr(INSTR_STORE, get_sizeof(this->type), value, SymbolList_get(func->all_vars,REG_GLOBALS), this->symbol));
    } else {
        Symbol sym = get_codegen_symbol(func, this->symbol);
        add_instr(func, new_Instr(INSTR_MOV, 0, sym, value, 0));
    }
}

// ---------------------------------------------------------------------------------
//                         code_gen_aggregate_lhs
// ---------------------------------------------------------------------------------
// Run code generation on an AST - interpreting as the lhs of an aggregate operation

Symbol  code_gen_aggregate_lhs_symbol(Function func, AST_symbol this) {
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
            fatal("Got kind %x in code_gen_aggregate_lhs_id", s->kind);
    }
}

// ---------------------------------------------------------------------------------
//                         code_gen_aggregate_rhs
// ---------------------------------------------------------------------------------
// Run code generation on an AST - interpreting as the lhs of an aggregate operation

void  code_gen_aggregate_rhs_symbol(Function func, AST_symbol this, Symbol dest) {
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
            fatal("Got kind %x in code_gen_aggregate_lhs_id", s->kind);
    }

    gen_memcopy(func, dest, addr,  get_sizeof(this->type));
}

