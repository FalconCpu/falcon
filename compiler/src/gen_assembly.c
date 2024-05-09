#include <stdio.h>
#include "fpl.h"
#include "instr.h"
 
// -----------------------------------------------------------------------
//                    gen_asm_alu
// -----------------------------------------------------------------------

#define reg_or_zero(A) ((A)->kind==SYM_REG || ((A)->kind==SYM_CONSTANT && (A)->value.int_val==0))
#define reg_or_small(A) ((A)->kind==SYM_REG || ((A)->kind==SYM_CONSTANT && ((A)->value.int_val>=-0x1000 || (A)->value.int_val<=0xfff)))

static void gen_asm_alu(FILE* file_out, Instr i) {
    assert(i->kind==INSTR_ALU);
    assert(reg_or_zero(i->dest));
    assert(reg_or_zero(i->a));
    assert(reg_or_small(i->b));

    switch(i->op) {
        case ALU_AND_I:     fprintf(file_out, "and %s, %s, %s\n", i->dest->name, i->a->name, i->b->name); break;
        case ALU_OR_I:      fprintf(file_out, "or  %s, %s, %s\n", i->dest->name, i->a->name, i->b->name); break;
        case ALU_XOR_I:     fprintf(file_out, "xor %s, %s, %s\n", i->dest->name, i->a->name, i->b->name); break;
        case ALU_LSL_I:     fprintf(file_out, "lsl %s, %s, %s\n", i->dest->name, i->a->name, i->b->name); break;
        case ALU_LSR_I:     fprintf(file_out, "lsr %s, %s, %s\n", i->dest->name, i->a->name, i->b->name); break;
        case ALU_ASR_I:     fprintf(file_out, "asr %s, %s, %s\n", i->dest->name, i->a->name, i->b->name); break;
        case ALU_ADD_I:     fprintf(file_out, "add %s, %s, %s\n", i->dest->name, i->a->name, i->b->name); break;
        case ALU_SUB_I:     fprintf(file_out, "sub %s, %s, %s\n", i->dest->name, i->a->name, i->b->name); break;
        case ALU_MUL_I:     if (i->b->kind==SYM_REG)
                                fprintf(file_out, "mul %s, %s, %s\n", i->dest->name, i->a->name, i->b->name); 
                            else {
                                fprintf(file_out, "ld  $29, %s\n", i->b->name); 
                                fprintf(file_out, "mul %s, %s, $29\n", i->dest->name, i->a->name); 
                            }
                            break;
        case ALU_DIV_I:     fprintf(file_out, "div %s, %s, %s\n", i->dest->name, i->a->name, i->b->name); break;
        case ALU_MOD_I:     fprintf(file_out, "mod %s, %s, %s\n", i->dest->name, i->a->name, i->b->name); break;

        case ALU_EQ_I:      fprintf(file_out, "xor  %s, %s, %s\n", i->dest->name, i->a->name, i->b->name); 
                            fprintf(file_out, "cltu %s, %s, 1\n", i->dest->name, i->dest->name); 
                            break;
        case ALU_NEQ_I:
        case ALU_LT_I:
        case ALU_LTE_I:
        case ALU_GT_I:
        case ALU_GTE_I:
        case ALU_ADD_F:
        case ALU_SUB_F:
        case ALU_MUL_F:
        case ALU_DIV_F:
        case ALU_MOD_F:
        case ALU_EQ_F:
        case ALU_NEQ_F:
        case ALU_LT_F:
        case ALU_LTE_F:
        case ALU_GT_F:
        case ALU_GTE_F:
        case ALU_ADD_S:
        case ALU_EQ_S:
        case ALU_NEQ_S:
        case ALU_LT_S:
        case ALU_LTE_S:
        case ALU_GT_S:
        case ALU_GTE_S:
        case ALU_AND_B:
        case ALU_OR_B:      fatal("Not yet implemented in gen_asm");

        default:    fatal("Got kind %x in gen_asm_alu", i->op);
    }
}


// -----------------------------------------------------------------------
//                    gen_asm_bra
// -----------------------------------------------------------------------

static void gen_asm_bra(FILE* file_out, Instr i) {
    switch(i->op) {
        case ALU_BEQ_I: fprintf(file_out,"beq %s, %s, .%s\n", i->a->name, i->b->name, i->dest->name+1); break;
        case ALU_BNE_I: fprintf(file_out,"bne %s, %s, .%s\n", i->a->name, i->b->name, i->dest->name+1); break;
        case ALU_BLT_I: fprintf(file_out,"blt %s, %s, .%s\n", i->a->name, i->b->name, i->dest->name+1); break;
        case ALU_BLE_I: fprintf(file_out,"bge %s, %s, .%s\n", i->b->name, i->a->name, i->dest->name+1); break;
        case ALU_BGT_I: fprintf(file_out,"blt %s, %s, .%s\n", i->b->name, i->a->name, i->dest->name+1); break;
        case ALU_BGE_I: fprintf(file_out,"bge %s, %s, .%s\n", i->a->name, i->b->name, i->dest->name+1); break;
        default: fatal ("Got kind %x in gen_asm_bra", i->op);
    }
}

// -----------------------------------------------------------------
//                      get_size_name
// -----------------------------------------------------------------

static int get_size_letter(int size) {
    assert(size==1 || size==2 || size==4);
    switch (size) {
        case 1: return 'b';
        case 2: return 'h';
        case 4: return 'w';
    }
    return '?';
}

// -----------------------------------------------------------------
//                      gen_preamble
// -----------------------------------------------------------------

static void gen_preamble(Function func, FILE* file_out) {
    fprintf(file_out,"%s:\n", func->name);

    int stack_frame = func->stack_vars;
    if (func->max_reg_no >= REG_CALLEE_SAVE)
        stack_frame += 4*(func->max_reg_no - REG_CALLEE_SAVE + 1);
    if (func->makes_calls)
        stack_frame += 4;
    
    if (stack_frame!=0)
        fprintf(file_out,"sub   $sp, %d\n", stack_frame);
    int offset = func->stack_vars;
    for(int reg = REG_CALLEE_SAVE; reg<=func->max_reg_no; reg++) {
        fprintf(file_out,"stw   $%d, $sp[%d]\n", reg, offset);
        offset += 4;
    }
    if (func->makes_calls)
        fprintf(file_out,"stw   $%d, $sp[%d]\n", REG_LINK, offset);
}


// -----------------------------------------------------------------
//                      gen_postamble
// -----------------------------------------------------------------

static void gen_postamble(Function func, FILE* file_out) {
    int stack_frame = func->stack_vars;
    if (func->max_reg_no >= REG_CALLEE_SAVE)
        stack_frame += 4*(func->max_reg_no - REG_CALLEE_SAVE + 1);
    if (func->makes_calls)
        stack_frame += 4;
    
    int offset = func->stack_vars;
    for(int reg = REG_CALLEE_SAVE; reg<=func->max_reg_no; reg++) {
        fprintf(file_out,"ldw   $%d, $sp[%d]\n", reg, offset);
        offset += 4;
    }
    if (func->makes_calls)
        fprintf(file_out,"ldw   $%d, $sp[%d]\n", REG_LINK, offset);
    if (stack_frame!=0)
        fprintf(file_out,"add   $sp, %d\n", stack_frame);
    
    if (func->body->parent==0)
        fprintf(file_out,"jmp main\n\n");
    else
        fprintf(file_out,"ret\n\n");
}

// -----------------------------------------------------------------
//                      output_escaped_string
// -----------------------------------------------------------------

static void output_escaped_string(FILE* file_out, String string) {
    fprintf(file_out, "%c", '"');
    string++;

    while(string[1]) {
        if (*string == '"') {
           fprintf(file_out, "%c", '\\');
           fprintf(file_out, "%c", '\"');
        } else if (*string == '\n') {
           fprintf(file_out, "%c", '\\');
           fprintf(file_out, "%c", '\n');
        } else 
           fprintf(file_out, "%c", *string);
        string ++;
    }
    fprintf(file_out, "%c", '"');
}


// -----------------------------------------------------------------
//                      generate_assembly
// -----------------------------------------------------------------

void generate_assembly(Function func, FILE *file_out) {
    Instr i;
    foreach_instr(i, func) {
        switch(i->kind) {
            case INSTR_NOP:     fprintf(file_out,"nop\n");
                                break;

            case INSTR_MOV:     assert(i->dest->kind==SYM_REG);
                                assert(i->a->kind==SYM_REG || i->a->kind==SYM_CONSTANT);
                                fprintf(file_out,"ld %s, %s\n", i->dest->name, i->a->name);
                                break;

            case INSTR_ALU:     gen_asm_alu(file_out, i);
                                break;

            case INSTR_LOAD:    assert(i->a->kind==SYM_REG);
                                if (i->b->kind==SYM_REG && i->b->index==0)
                                    fprintf(file_out,"ld%c %s, %s[%d]\n", get_size_letter(i->op), i->dest->name, i->a->name, 0);
                                else if (i->b->kind==SYM_GLOBALVAR || i->b->kind==SYM_FIELD)
                                    fprintf(file_out,"ld%c %s, %s[%d]\n", get_size_letter(i->op), i->dest->name, i->a->name, i->b->offset);
                                else if (i->b->kind==SYM_CONSTANT)
                                    fprintf(file_out,"ld%c %s, %s[%d]\n", get_size_letter(i->op), i->dest->name, i->a->name, i->b->value.int_val);
                                else
                                    fatal("Got kind %x in INSTR_LOAD",i->b->kind);
                                break;

            case INSTR_STORE:   assert(i->a->kind==SYM_REG);
                                if (i->b->kind==SYM_REG && i->b->index==0)
                                    fprintf(file_out,"st%c %s, %s[%d]\n", get_size_letter(i->op), i->dest->name, i->a->name, 0);
                                else if (i->b->kind==SYM_GLOBALVAR || i->b->kind==SYM_FIELD)
                                    fprintf(file_out,"st%c %s, %s[%d]\n", get_size_letter(i->op), i->dest->name, i->a->name, i->b->offset);
                                else if (i->b->kind==SYM_CONSTANT)
                                    fprintf(file_out,"st%c %s, %s[%d]\n", get_size_letter(i->op), i->dest->name, i->a->name, i->b->value.int_val);
                                else
                                    fatal("Got kind %x in INSTR_STORE",i->b->kind);
                                break;

            case INSTR_BRANCH:  gen_asm_bra(file_out, i);
                                break;

            case INSTR_JUMP:    fprintf(file_out,"jmp  .%s\n", i->dest->name+1);
                                break;

            case INSTR_LABEL:   fprintf(file_out,".%s:\n", i->dest->name+1);
                                break;

            case INSTR_CALL:    fprintf(file_out, "jsr  %s\n", i->a->name);
                                break;

            case INSTR_LEA:     fprintf(file_out, "ld    %s, ", i->dest->name);
                                if (i->a->kind==SYM_CONSTANT && i->a->type==type_string)
                                    output_escaped_string(file_out, i->a->name);
                                else
                                    fprintf(file_out,"%s",i->a->name);
                                fprintf(file_out, "\n");
                                break;

            case INSTR_CHK:     // for now do no bounds checking
                                break;

            case INSTR_START:   gen_preamble(func, file_out);
                                break;

            case INSTR_END:     gen_postamble(func, file_out);
                                break;
        }
    }
}