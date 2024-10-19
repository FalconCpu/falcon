#include <stdio.h>
#include "fpl.h"
#include "AST.h"


// ============================================================================
//                          CONSTRUCTOR
// ============================================================================

AST new_AST_assignOp(Location location, TokenKind op, AST lhs, AST rhs) {
    AST_assignOp ret = new(struct AST_assignOp);
    ret->location = location;
    ret->kind     = AST_ASSIGNOP;
    ret->op       = op;
    ret->lhs      = lhs;
    ret->rhs      = rhs;
    return as_AST(ret);
}

// ============================================================================
//                          CAST
// ============================================================================

AST_assignOp as_assignOp(AST this) {
    assert(this->kind==AST_ASSIGNOP);
    return (AST_assignOp) this;
}


// ============================================================================
//                          PRINT
// ============================================================================

void AST_assignOp_print(AST_assignOp this, int indent) {
    print_spaces(indent);
    printf("ASSIGN%s\n", token_kind_names[this->op]);
    AST_print(this->lhs, indent+1);
    AST_print(this->rhs, indent+1);
}

// ============================================================================
//                         typecheck
// ============================================================================

void AST_typecheck_assignOp(AST_assignOp this, Block scope) {
    AST_typecheck_value(this->lhs, scope);
    AST_typecheck_value(this->rhs, scope);
    check_lvalue(this->lhs);
    check_type_compatible(this->lhs->type, this->rhs);
    check_type_compatible(type_int, this->rhs);
}


// ============================================================================
//                           code_gen
// ============================================================================

Symbol code_gen_assignOp(Function func, AST_assignOp this) {
    Symbol rhs = code_gen(func, this->rhs);

    AluOp aluOp;
    if (this->op==TOK_PLUSEQ)
        aluOp = ALU_ADD_I;
    else if (this->op==TOK_MINUSEQ)
        aluOp = ALU_SUB_I;
    else
        fatal("Got kind %x in code_gen_assignOp", this->op);
    
    if (this->lhs->kind==AST_SYMBOL && as_symbol(this->lhs)->symbol->kind==SYM_VARIABLE) {
        Symbol lhs = as_symbol(this->lhs)->symbol;
        add_instr(func, new_Instr(INSTR_ALU, aluOp, lhs, lhs, rhs));
    } else {
        Symbol lhs = code_gen_address_of(func, this->lhs);
        Symbol tmp1 = new_tempvar(func, this->rhs->type);
        Symbol tmp2 = new_tempvar(func, this->rhs->type);
        Symbol zero = make_constant_symbol(func,0);
        add_instr(func, new_Instr(INSTR_LOAD, get_sizeof(rhs->type), tmp1, lhs, zero));
        add_instr(func, new_Instr(INSTR_ALU, aluOp, tmp2, tmp1, rhs));
        add_instr(func, new_Instr(INSTR_STORE, get_sizeof(rhs->type), tmp2, lhs, zero));
    }
    return rhs;
}
