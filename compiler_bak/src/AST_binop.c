#include <stdio.h>
#include "fpl.h"
#include "AST.h"
#include "instr.h"
#include "types.h"


// ============================================================================
//                          CONSTRUCTOR
// ============================================================================

AST new_AST_binop(Location location, TokenKind op, AST lhs, AST rhs) {
    AST_binop ret = new(struct AST_binop);
    ret->location = location;
    ret->kind     = AST_BINOP;
    ret->op       = op;
    ret->lhs      = lhs;
    ret->rhs      = rhs;
    return as_AST(ret);
}

// ============================================================================
//                          CAST
// ============================================================================

AST_binop as_binop(AST this) {
    assert(this->kind==AST_BINOP);
    return (AST_binop) this;
}


// ============================================================================
//                          PRINT
// ============================================================================

void AST_binop_print(AST_binop this, int indent) {
    print_spaces(indent);
    printf("BINOP %s %s\n", token_kind_names[this->op], this->type ? this->type->name : "");
    AST_print(this->lhs, indent+1);
    AST_print(this->rhs, indent+1);
}

// ---------------------------------------------------------------------------------
//                         operators
// ---------------------------------------------------------------------------------

struct binops {
    TokenKind op;
    TypeKind lhs_type;
    TypeKind rhs_type;
    AluOp    out_op;
    TypeKind out_type;
};

static struct binops binop_table[] = {
    {TOK_PLUS,      TYPE_INT,  TYPE_INT,  ALU_ADD_I,  TYPE_INT},
    {TOK_MINUS,     TYPE_INT,  TYPE_INT,  ALU_SUB_I,  TYPE_INT},
    {TOK_STAR,      TYPE_INT,  TYPE_INT,  ALU_MUL_I,  TYPE_INT},
    {TOK_SLASH,     TYPE_INT,  TYPE_INT,  ALU_DIV_I,  TYPE_INT},
    {TOK_PERCENT,   TYPE_INT,  TYPE_INT,  ALU_MOD_I,  TYPE_INT},
    {TOK_AMPERSAND, TYPE_INT,  TYPE_INT,  ALU_AND_I,  TYPE_INT},
    {TOK_BAR,       TYPE_INT,  TYPE_INT,  ALU_OR_I,   TYPE_INT},
    {TOK_CARAT,     TYPE_INT,  TYPE_INT,  ALU_XOR_I,  TYPE_INT},
    {TOK_LEFT,      TYPE_INT,  TYPE_INT,  ALU_LSL_I,  TYPE_INT},
    {TOK_RIGHT,     TYPE_INT,  TYPE_INT,  ALU_LSR_I,  TYPE_INT},
    {TOK_EQ,        TYPE_INT,  TYPE_INT,  ALU_EQ_I,   TYPE_BOOL},
    {TOK_NEQ,       TYPE_INT,  TYPE_INT,  ALU_NEQ_I,  TYPE_BOOL},
    {TOK_LT,        TYPE_INT,  TYPE_INT,  ALU_LT_I,   TYPE_BOOL},
    {TOK_LTE,       TYPE_INT,  TYPE_INT,  ALU_LTE_I,  TYPE_BOOL},
    {TOK_GT,        TYPE_INT,  TYPE_INT,  ALU_GT_I,   TYPE_BOOL},
    {TOK_GTE,       TYPE_INT,  TYPE_INT,  ALU_GTE_I,  TYPE_BOOL},

    {TOK_PLUS,      TYPE_REAL, TYPE_REAL,  ALU_ADD_F,  TYPE_REAL},
    {TOK_MINUS,     TYPE_REAL, TYPE_REAL,  ALU_SUB_F,  TYPE_REAL},
    {TOK_STAR,      TYPE_REAL, TYPE_REAL,  ALU_MUL_F,  TYPE_REAL},
    {TOK_SLASH,     TYPE_REAL, TYPE_REAL,  ALU_DIV_F,  TYPE_REAL},
    {TOK_PERCENT,   TYPE_REAL, TYPE_REAL,  ALU_MOD_F,  TYPE_REAL},
    {TOK_EQ,        TYPE_REAL, TYPE_REAL,  ALU_EQ_F,   TYPE_BOOL},
    {TOK_NEQ,       TYPE_REAL, TYPE_REAL,  ALU_NEQ_F,  TYPE_BOOL},
    {TOK_LT,        TYPE_REAL, TYPE_REAL,  ALU_LT_F,   TYPE_BOOL},
    {TOK_LTE,       TYPE_REAL, TYPE_REAL,  ALU_LTE_F,  TYPE_BOOL},
    {TOK_GT,        TYPE_REAL, TYPE_REAL,  ALU_GT_F,   TYPE_BOOL},
    {TOK_GTE,       TYPE_REAL, TYPE_REAL,  ALU_GTE_F,  TYPE_BOOL},

    {TOK_PLUS,      TYPE_STRING, TYPE_STRING, ALU_ADD_S, TYPE_STRING},
    {TOK_EQ,        TYPE_STRING, TYPE_STRING, ALU_EQ_S,  TYPE_BOOL},
    {TOK_NEQ,       TYPE_STRING, TYPE_STRING, ALU_NEQ_S, TYPE_BOOL},
    {TOK_LT,        TYPE_STRING, TYPE_STRING, ALU_LT_S,  TYPE_BOOL},
    {TOK_LTE,       TYPE_STRING, TYPE_STRING, ALU_LTE_S, TYPE_BOOL},
    {TOK_GT,        TYPE_STRING, TYPE_STRING, ALU_GT_S,  TYPE_BOOL},
    {TOK_GTE,       TYPE_STRING, TYPE_STRING, ALU_GTE_S, TYPE_BOOL},

    {TOK_EQ,        TYPE_BOOL,  TYPE_BOOL,  ALU_EQ_I,   TYPE_BOOL},
    {TOK_NEQ,       TYPE_BOOL,  TYPE_BOOL,  ALU_NEQ_I,  TYPE_BOOL},
    {TOK_LT,        TYPE_BOOL,  TYPE_BOOL,  ALU_LT_I,   TYPE_BOOL},
    {TOK_LTE,       TYPE_BOOL,  TYPE_BOOL,  ALU_LTE_I,  TYPE_BOOL},
    {TOK_GT,        TYPE_BOOL,  TYPE_BOOL,  ALU_GT_I,   TYPE_BOOL},
    {TOK_GTE,       TYPE_BOOL,  TYPE_BOOL,  ALU_GTE_I,  TYPE_BOOL},

    {TOK_AND,       TYPE_BOOL, TYPE_BOOL,    ALU_AND_B,  TYPE_BOOL},
    {TOK_OR,        TYPE_BOOL, TYPE_BOOL,    ALU_OR_B,   TYPE_BOOL},

    {TOK_EQ,        TYPE_POINTER,  TYPE_POINTER,  ALU_EQ_I,   TYPE_BOOL},
    {TOK_NEQ,       TYPE_POINTER,  TYPE_POINTER,  ALU_NEQ_I,  TYPE_BOOL},
    {TOK_EQ,        TYPE_POINTER,  TYPE_NULL,     ALU_EQ_I,   TYPE_BOOL},
    {TOK_NEQ,       TYPE_POINTER,  TYPE_NULL,     ALU_NEQ_I,  TYPE_BOOL},
    {TOK_EQ,        TYPE_NULL,     TYPE_POINTER,  ALU_EQ_I,   TYPE_BOOL},
    {TOK_NEQ,       TYPE_NULL,     TYPE_POINTER,  ALU_NEQ_I,  TYPE_BOOL},

    {0,0,0,0,0}
};

// ---------------------------------------------------------------------------------
//                         compiletime_eval
// ---------------------------------------------------------------------------------
// evaluate a binop at compile time if possible

static void compiletime_eval(AST_binop this) {
    if (this->lhs->kind!=AST_INTLIT || this->rhs->kind!=AST_INTLIT)
        return;

    int l = as_intlit(this->lhs)->value;
    int r = as_intlit(this->rhs)->value;
    int ret;
    switch(this->alu_op) {
        case ALU_ADD_I: ret = l + r; break;
        case ALU_SUB_I: ret = l - r; break;
        case ALU_MUL_I: ret = l * r; break;
        case ALU_DIV_I: ret = l / r; break;
        case ALU_MOD_I: ret = l % r; break;
        case ALU_AND_I: ret = l & r; break;
        case ALU_OR_I:  ret = l | r; break;
        case ALU_XOR_I: ret = l ^ r; break;
        case ALU_EQ_I:  ret = l == r; break;
        case ALU_NEQ_I: ret = l != r; break;
        case ALU_LT_I:  ret = l < r; break;
        case ALU_LTE_I: ret = l <= r; break;
        case ALU_GT_I:  ret = l > r; break;
        case ALU_GTE_I: ret = l >= r; break;
        default:  return;
    }

    // bit of an ugly hack here - but overwrite the AST_binop with an AST_intlit
    AST_intlit this_intlit = (AST_intlit) this;
    this_intlit->kind = AST_INTLIT;
    this_intlit->value = ret;
    return;
}


// ---------------------------------------------------------------------------------
//                         typeCheck
// ---------------------------------------------------------------------------------
// Typecheck a binary operator

void AST_typecheck_binop(AST_binop this, Block scope) {
    AST_typecheck_value(this->lhs, scope);
    AST_typecheck_value(this->rhs, scope);

    if (this->lhs->type==type_error || this->rhs->type==type_error) {
        this->type = type_error;
        return;
    }

    // perform automatic casts of char/short to int if used in arithmetic
    if (this->lhs->type==type_char || this->lhs->type==type_short) {
        this->lhs = new_AST_cast(this->location, this->lhs, 0);
        this->lhs->type = type_int;
    }
    if (this->rhs->type==type_char || this->rhs->type==type_short) {
        this->rhs = new_AST_cast(this->location, this->rhs, 0);
        this->rhs->type = type_int;
    }

    // if types are pointers then check their base types are the same
    if (this->lhs->type->kind==TYPE_POINTER && this->rhs->type->kind==TYPE_POINTER)
        if (as_TypePointer(this->lhs->type)->base != as_TypePointer(this->rhs->type)->base)
            error(this->location, "Pointer type mismatch, %s vs %s", this->lhs->type->name, this->rhs->type->name);

    struct binops *ptr;
    for(ptr=binop_table; ptr->op; ptr++)
        if (ptr->op==this->op && ptr->lhs_type==this->lhs->type->kind && ptr->rhs_type==this->rhs->type->kind) {
            this->alu_op = ptr->out_op;
            this->type = get_type_from_code(ptr->out_type);
            compiletime_eval(this);
            return;
        }

    this->type = make_type_error(this->location, "No operation defined for %s %s %s", this->lhs->type->name, token_kind_names[this->op], this->rhs->type->name);
    this->op = 0;
}

// ---------------------------------------------------------------------------------
//                           code_gen
// ---------------------------------------------------------------------------------

Symbol code_gen_binop(Function func, AST_binop this) {
    Symbol lhs = code_gen(func, this->lhs);
    Symbol rhs = code_gen(func, this->rhs);
    Symbol ret = new_tempvar(func, this->type);
    add_instr(func, new_Instr(INSTR_ALU, this->alu_op, ret, lhs, rhs));
    return ret;
}

// ---------------------------------------------------------------------------------
//                           code_gen_bool
// ---------------------------------------------------------------------------------

void  code_gen_bool_binop(Function func, AST_binop this, Symbol label_true, Symbol label_false) {
    if (this->alu_op>=ALU_EQ_I && this->alu_op<=ALU_GTE_I) {
        // comparison ops can be implemented as branch statements
        Symbol lhs = code_gen(func, this->lhs);
        Symbol rhs = code_gen(func, this->rhs);
        add_instr(func, new_Instr(INSTR_BRANCH, ALU_BEQ_I + (this->alu_op-ALU_EQ_I), label_true, lhs, rhs));
        add_instr(func, new_Instr(INSTR_JUMP,  0, label_false,0, 0));

    } else if (this->alu_op==ALU_AND_B) {
        Symbol lab = new_label(func);
        code_gen_bool(func, this->lhs, lab, label_false);
        add_instr(func, new_Instr(INSTR_LABEL, 0, lab, 0, 0));
        code_gen_bool(func, this->rhs, label_true, label_false);
    } else if (this->alu_op==ALU_OR_B) {
        Symbol lab = new_label(func);
        code_gen_bool(func, this->lhs, label_true, lab);
        add_instr(func, new_Instr(INSTR_LABEL, 0, lab, 0, 0));
        code_gen_bool(func, this->rhs, label_true, label_false);
    } else {
        Symbol v = code_gen(func, as_AST(this));
        add_instr(func, new_Instr(INSTR_BRANCH, ALU_BNE_I, label_true, v, make_constant_symbol(func, 0)));
        add_instr(func, new_Instr(INSTR_JUMP,  0, label_false, 0, 0));

    }
}

