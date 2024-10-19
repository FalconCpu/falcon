#include <stdio.h>
#include "fpl.h"
#include "instr.h"
#include "types.h"

extern int debug;
static int made_change = 0;

// --------------------------------------------------------------------
//                             get_usecounts
// --------------------------------------------------------------------

static void get_use_counts(Function func) {
    Symbol s;
    foreach_symbol(s, func->all_vars) {
        s->read_count = 0;
        s->write_count = 0;
        s->index = index;
        s->copy_of = 0;
    }

    // for labels write_count=number of times they are target of a jump/branch
    // index = index of instuction which places them
    foreach_symbol(s, func->all_labels) {
        s->read_count = 0;
        s->write_count = 0;
        s->index = 0;
    }

    foreach_symbol(s, func->all_consts) {
        s->read_count = 0;
        s->write_count = 0;
        s->index = index;
    }

    Instr i;
    foreach_instr(i, func) {
        i->index = index;
        switch(i->kind) {
            case INSTR_NOP:     break;
            case INSTR_MOV:     i->dest->write_count++; i->a->read_count++;
                                i->dest->copy_of = i->a;
                                break;
            case INSTR_ALU:     i->dest->write_count++; i->a->read_count++; i->b->read_count++; break;
            case INSTR_LOAD:    i->dest->write_count++; i->a->read_count++; i->b->read_count++; break;
            case INSTR_STORE:   i->dest->read_count++;  i->a->read_count++; i->b->read_count++; break;
            case INSTR_BRANCH:  i->dest->write_count++; i->a->read_count++; i->b->read_count++; break;
            case INSTR_JUMP:    i->dest->write_count++; break;
            case INSTR_LABEL:   i->dest->index = index; break;
            case INSTR_CALL:    for(int j=REG_ARGS; j< REG_ARGS+ as_TypeFunction(i->a->type)->parameter_types->count + symbol_is_member_function(i->a); j++)
                                    SymbolList_get(func->all_vars, j) -> read_count ++;
                                break;
            case INSTR_LEA:     i->dest->write_count++; i->a->read_count++; break;
            case INSTR_CHK:     i->a->read_count++; i->b->read_count++; break;
            case INSTR_START:   for (int i=0; i<func->params->count; i++)
                                     SymbolList_get(func->all_vars, i+1)->write_count ++;
                                 break;
            case INSTR_END:     if (func->type!=type_void)
                                     SymbolList_get(func->all_vars, 8)->read_count ++;
                                break;
        }
    }
}

// --------------------------------------------------------------------
//                             change_to_nop
// --------------------------------------------------------------------

static void change_to_nop(Instr this) {
    if (this->kind==INSTR_NOP)
        return;
    if (debug) printf("Changing instruction %d to nop\n", this->index);
    this->kind = INSTR_NOP;
    this->dest = 0;
    this->b    = 0;
    made_change = 1;
}

// --------------------------------------------------------------------
//                             invert_branch
// --------------------------------------------------------------------

static void invert_branch(Instr i, Symbol new_dest) {
    assert(i->kind==INSTR_BRANCH);
    if (debug) printf("Inverting branch %d to %s\n", i->index, new_dest->name);
    switch(i->op) {
        case ALU_BEQ_I: i->op = ALU_BNE_I; break;
        case ALU_BNE_I: i->op = ALU_BEQ_I; break;
        case ALU_BLT_I: i->op = ALU_BGE_I; break;
        case ALU_BGE_I: i->op = ALU_BLT_I; break;
        case ALU_BGT_I: i->op = ALU_BLE_I; break;
        case ALU_BLE_I: i->op = ALU_BGT_I; break;
        default: fatal("Got branch op=%x in invert_branch", i->op);
    }
    i->dest = new_dest;
    new_dest->write_count ++;
    made_change = 1;
}

// --------------------------------------------------------------------
//                            change_to
// --------------------------------------------------------------------

static void change_to(Instr this, int kind, AluOp op, Symbol dest, Symbol a, Symbol b) {
    this->kind = kind;
    this->op = op;
    this->dest = dest;
    this->a = a;
    this->b = b;
    if (debug) printf("Changed %d to %s\n",this->index, alu_names[op]);
    made_change = 1;
}


// --------------------------------------------------------------------
//                     log2
// --------------------------------------------------------------------

static int log2(int a) {
    int ret = 0;
    while(a>=2) {
        ret++;
        a= a >> 1;
    }
    return ret;
}

static int isPowerOf2(int a) {
    if ( a>0 && ((a-1)&a) ==0 )
        return 1;
    else
        return 0;
}



// --------------------------------------------------------------------
//                     arith_ops
// --------------------------------------------------------------------

static void arith_ops(Function func, Instr this)  {
    assert(this->b->kind==SYM_CONSTANT);

    int b_val = this->b->value.int_val;

    switch(this->op) {
        case ALU_ADD_I:
        case ALU_SUB_I:
        case ALU_OR_I:
        case ALU_XOR_I:
            if (b_val==0)
                change_to(this, INSTR_MOV, 0, this->dest, this->a, 0);
            break;

        case ALU_AND_I:
            if (b_val==0)
                change_to(this, INSTR_MOV, 0, this->dest, make_constant_symbol(func,0), 0);
            break;

        case ALU_MUL_I:
            if (b_val==0)
                change_to(this, INSTR_MOV, 0, this->dest, make_constant_symbol(func,0), 0);
            else if (b_val==1)
                change_to(this, INSTR_MOV, 0, this->dest, this->a, 0);
            else if (isPowerOf2(b_val))
                change_to(this, INSTR_ALU, ALU_LSL_I, this->dest, this->a, make_constant_symbol(func,log2(b_val)));
            break;

        case ALU_DIV_I:
            if (b_val==1)
                change_to_nop(this);
            else if (isPowerOf2(b_val))
                change_to(this, INSTR_ALU, ALU_ASR_I, this->dest, this->a, make_constant_symbol(func,log2(b_val)));
            break;

        case ALU_MOD_I:
            if (isPowerOf2(b_val))
                change_to(this, INSTR_ALU, ALU_AND_I, this->dest, this->a, make_constant_symbol(func,b_val-1));
            break;

        default: break;
    }
}




// --------------------------------------------------------------------
//                     peephole_pass
// --------------------------------------------------------------------

static void peephole_pass(Function func) {
    Instr i;
    foreach_instr(i, func) {
        switch(i->kind) {
            case INSTR_NOP:     break;

            case INSTR_MOV:     if (i->dest->read_count==0)
                                    change_to_nop(i);
                                else if (i->a->read_count==1 && func->instr[index-1]->dest==i->a) {
                                    if (debug) printf("Changed %d.dest to %s", index-1, i->dest->name);
                                    func->instr[index-1]->dest = i->dest;
                                    change_to_nop(i);
                                } else if (i->dest==i->a)
                                    change_to_nop(i);
                                break;

            case INSTR_ALU:     if (i->dest->read_count==0)
                                    change_to_nop(i);
                                else if (i->b->write_count==1 && i->b->copy_of!=0 && i->b->copy_of->kind==SYM_CONSTANT) {
                                    int val = i->b->copy_of->value.int_val;
                                    if (val>=-0xfff && val<=0xfff) {
                                        if (debug) printf("Changing %d.b to %s", i->index, i->b->copy_of->name);
                                        made_change = 1;
                                        i->b = i->b->copy_of;
                                    }
                                } else if (i->b->kind==SYM_CONSTANT)
                                    arith_ops(func, i);
                                break;

            case INSTR_LOAD:    if (i->dest->read_count==0)
                                    change_to_nop(i);
                                break;

            case INSTR_STORE:   break;

            case INSTR_BRANCH:  if (i->dest->index==index+2 && func->instr[index+1]->kind==INSTR_JUMP) {
                                    invert_branch(i, func->instr[index+1]->dest);
                                    change_to_nop(func->instr[index+1]);
                                }
                                if (i->a->write_count==1 && i->a->copy_of!=0 && i->a->copy_of->kind==SYM_CONSTANT && i->a->copy_of->value.int_val==0) {
                                    if (debug) printf("Changed %d.a to 0\n", i->index);
                                    i->a = SymbolList_get(func->all_vars,0);
                                    made_change = 1;
                                }
                                if (i->b->write_count==1 && i->b->copy_of!=0 && i->b->copy_of->kind==SYM_CONSTANT && i->b->copy_of->value.int_val==0) {
                                    if (debug) printf("Changed %d.b to 0\n", i->index);
                                    i->b = SymbolList_get(func->all_vars,0);
                                    made_change = 1;
                                }
                                break;

            case INSTR_JUMP:    if (i->dest->index == i->index+1)
                                    change_to_nop(i);
                                break;

            case INSTR_LABEL:   if (i->dest->write_count==0)
                                    change_to_nop(i);
                                break;

            case INSTR_CALL:    break;

            case INSTR_CHK:     break;

            case INSTR_LEA:     if (i->dest->read_count==0)
                                    change_to_nop(i);
                                break;

            case INSTR_START:   break;

            case INSTR_END:     break;
        }
    }
}

// --------------------------------------------------------------------
//                     remove unreachable
// --------------------------------------------------------------------

static void remove_unreachable(Function func) {
    int reachable = 1;
    Instr i;
    foreach_instr(i, func) {
        if (i->kind==INSTR_LABEL)
            reachable = 1;
        if (reachable==0)
            change_to_nop(i);
        if (i->kind==INSTR_JUMP)
            reachable = 0;
    }
}


// --------------------------------------------------------------------
//                     remove nop
// --------------------------------------------------------------------

static void remove_nop(Function func) {
    int dst = 0;
    int src = 0;
    for(src = 0; src<func->num_instr; src++)
        if (func->instr[src]->kind != INSTR_NOP)
            func->instr[dst++] = func->instr[src];
    func->num_instr = dst;
}



// --------------------------------------------------------------------
//                             peephole
// --------------------------------------------------------------------
// Runs peephole (basic optimisation) on a the TAC code for a function

void peephole(Function func) {

    get_use_counts(func);
    do {
        made_change = 0;
        if (debug)
            print_program(func);
        peephole_pass(func);
        remove_unreachable(func);
        remove_nop(func);
        get_use_counts(func);
    } while (made_change);

    gen_livemap(func);

    if (debug)
        printf("Peephole completed\n");
}