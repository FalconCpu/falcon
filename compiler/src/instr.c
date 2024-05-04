#include <stdio.h>
#include "fpl.h"
#include "instr.h"

// -----------------------------------------------------------------
//                      Operator names
// -----------------------------------------------------------------

String alu_names[] = {
    [ALU_AND_I] = "and_i",
    [ALU_OR_I]  = "or_i",
    [ALU_XOR_I] = "xor_i",
    [ALU_LSL_I] = "lsl_i",
    [ALU_LSR_I] = "lsr_i",
    [ALU_ASR_I] = "asr_i",
    [ALU_ADD_I] = "add_i",
    [ALU_SUB_I] = "sub_i",
    [ALU_MUL_I] = "mul_i",
    [ALU_DIV_I] = "div_i",
    [ALU_MOD_I] = "mod_i",
    [ALU_EQ_I]  = "eq_i",
    [ALU_NEQ_I] = "neq_i",
    [ALU_LT_I]  = "lt_i",
    [ALU_LTE_I] = "lte_i",
    [ALU_GT_I]  = "gt_i",
    [ALU_GTE_I] = "gte_i",
    [ALU_BEQ_I] = "beq_i",
    [ALU_BNE_I] = "bne_i",
    [ALU_BLT_I] = "blt_i",
    [ALU_BGE_I] = "bge_i",
    [ALU_BLE_I] = "ble_i",
    [ALU_BGT_I] = "bgt_i",
    [ALU_AND_B] = "and_b",
    [ALU_OR_B]  = "or_b",
    [ALU_ADD_F] = "add_f",
    [ALU_SUB_F] = "sub_f",
    [ALU_MUL_F] = "mul_f",
    [ALU_DIV_F] = "div_f",
    [ALU_MOD_F] = "mod_f",
    [ALU_EQ_F]  = "eq_f",
    [ALU_NEQ_F] = "neq_f",
    [ALU_LT_F]  = "lt_f",
    [ALU_LTE_F] = "lte_f",
    [ALU_GT_F]  = "gt_f",
    [ALU_GTE_F] = "gte_f",
    [ALU_ADD_S] = "add_s",
    [ALU_EQ_S]  = "eq_s",
    [ALU_NEQ_S] = "neq_s",
    [ALU_LT_S]  = "lt_s",
    [ALU_LTE_S] = "lte_s",
    [ALU_GT_S]  = "gt_s",
    [ALU_GTE_S] = "gte_s",
};

// -----------------------------------------------------------------
//                      Constructor
// -----------------------------------------------------------------

Instr new_Instr(InstrKind kind, AluOp op, Symbol dest, Symbol a, Symbol b) {
    Instr ret = new(struct Instr);
    ret->kind = kind;
    ret->op = op;
    ret->dest = dest;
    ret->a = a;
    ret->b = b;
    return ret;
}

// -----------------------------------------------------------------
//                      get_size_name
// -----------------------------------------------------------------

static int get_size_letter(int size) {
    switch (size) {
        case 1: return 'b';
        case 2: return 'h';
        case 4: return 'w';
        default: return '?';
    }
}


// -----------------------------------------------------------------
//                      Instr_toString
// -----------------------------------------------------------------

String instr_toString(Instr this) {
    static char buf[80];
    switch(this->kind) {
        case INSTR_NOP:     sprintf(buf,"%-3d  nop", this->index); break;
        case INSTR_MOV:     sprintf(buf,"%-3d  mov   %s, %s", this->index, this->dest->name, this->a->name); break;
        case INSTR_ALU:     sprintf(buf,"%-3d  %-5s %s, %s, %s", this->index,alu_names[this->op], this->dest->name, this->a->name, this->b->name); break;
        case INSTR_LEA:     sprintf(buf,"%-3d  lea   %s, %s", this->index, this->dest->name, this->a->name); break;
        case INSTR_LOAD:    sprintf(buf,"%-3d  ld%c   %s, %s[%s]", this->index, get_size_letter(this->op), this->dest->name, this->a->name, this->b->name); break;
        case INSTR_STORE:   sprintf(buf,"%-3d  st%c   %s, %s[%s]", this->index, get_size_letter(this->op), this->dest->name, this->a->name, this->b->name); break;
        case INSTR_BRANCH:  sprintf(buf,"%-3d  %-5s %s, %s, %s", this->index, alu_names[this->op], this->a->name, this->b->name, this->dest->name); break;
        case INSTR_JUMP:    sprintf(buf,"%-3d  jmp   %s", this->index, this->dest->name); break;
        case INSTR_LABEL:   sprintf(buf,"%-3d  lab   %s", this->index, this->dest->name); break;
        case INSTR_CALL:    sprintf(buf,"%-3d  call  %s", this->index, this->a->name); break;
        case INSTR_CHK:     sprintf(buf,"%-3d  chk   %s, %s",  this->index, this->a->name, this->b->name); break;
        case INSTR_START:   sprintf(buf,"%-3d  start", this->index); break;
        case INSTR_END:     sprintf(buf,"%-3d  end", this->index); break;
    }
    return buf;
}