#include <stdio.h>
#include "fpl.h"
#include "instr.h"

extern int debug;

// magic number to indicate an unallocated variable
#define UNALLOCATED -1

// --------------------------------------------------------
//             build_livemap
// --------------------------------------------------------
// Build a bitmap where each set point represents a pair of variables
// which interfere with each other

static Bitmap build_interferemap(Function func) {
    Bitmap interfere = new_Bitmap(func->all_vars->count, func->all_vars->count);
    Instr i;
    foreach_instr(i, func) {
        switch(i->kind) {
            case INSTR_LOAD:
            case INSTR_ALU:
            case INSTR_LEA:
                // These instructions generate an interference between the destination variable and 
                // any variables that are live at the time
                bitmap_or_row(interfere, i->dest->index, func->livemap, index);
                break;

            case INSTR_MOV:
                // A move instruction generates an interference between all variables that are live except
                // for the source variable (As, by definition, after a move the source and destination have the
                // same value. Therefore they can be in the same register, unless prevented by some other instruction)
                //
                // TODO - maybe should write a 'bitmap_or_except_bit' function to remove this loop
                for(int j=0; j<func->all_vars->count; j++)
                    if (bitmap_get(func->livemap, i->index, j) && j!=i->a->index)
                        bitmap_set(interfere, i->dest->index, j);
                break;

            case INSTR_CALL:
                // Registers numbered REG_ARGS..REG_RESULT may be corrupted by the called function. So anything live at
                // the call site must not be in any of those registers.
                func->makes_calls = 1;
                for(int j=REG_ARGS; j<=REG_RESULT; j++)
                    bitmap_or_row(interfere, j, func->livemap, index);
                break;

            case INSTR_NOP:
            case INSTR_STORE:
            case INSTR_BRANCH:
            case INSTR_JUMP:
            case INSTR_LABEL:
            case INSTR_CHK:
            case INSTR_START:
            case INSTR_END:
                // these instructions do not write to any register
                break;
        }
    }

    // Make the interfere map symetric. 
    // TODO - look for a more efficient way to do this
    for(int y=0; y<interfere->num_cols; y++) 
        for(int x=0; x<interfere->num_cols; x++)
            if (bitmap_get(interfere, y, x))
                bitmap_set(interfere, x, y);
    
    return interfere;
}

// --------------------------------------------------------
//             get_candidate_for
// --------------------------------------------------------
// find a possible register for variable number var_number
// return 0 if none found

static int get_candidate_reg(Bitmap interfere, int var_number) {
    for(int i=1; i<REG_MAX; i++)
        if (! bitmap_get(interfere, i, var_number))
            return i;

    return 0;
}

// --------------------------------------------------------
//             allocate register
// --------------------------------------------------------
// allocate a variable to a register

static void allocate_register(Function func, Bitmap interfere, Symbol reg, Symbol var) {
    assert( reg->kind == SYM_REG);
    assert( var->kind == SYM_VARIABLE);

    if (debug)
        printf("Allocating %s to %s\n", var->name, reg->name);

    if (reg->index > func->max_reg_no && reg->index<=REG_MAX)
        func->max_reg_no = reg->index;

    var->reg_number = reg->reg_number;
    bitmap_or_row(interfere, reg->reg_number, interfere, var->index);
}

// --------------------------------------------------------
//             attempt_combine_move
// --------------------------------------------------------

#define is_var_or_reg(A) ((A)->kind==SYM_VARIABLE || (A)->kind==SYM_REG)

static void attempt_combine_move(Function func, Bitmap interfere) {
    Instr i;
    foreach_instr(i,func) {
        if (i->kind==INSTR_MOV && is_var_or_reg(i->a)) {
            if (i->dest->reg_number==UNALLOCATED && 
                i->a->reg_number != UNALLOCATED && 
                !bitmap_get(interfere, i->a->reg_number, i->dest->index) )
                allocate_register(func, interfere, SymbolList_get(func->all_vars, i->a->reg_number), i->dest);

            if (i->a->reg_number==UNALLOCATED && 
                i->dest->reg_number!=UNALLOCATED &&
                !bitmap_get(interfere, i->dest->reg_number, i->a->index) )
                allocate_register(func, interfere, SymbolList_get(func->all_vars, i->dest->reg_number), i->a);
        }
    }
}

// --------------------------------------------------------
//             print_interfere_map
// --------------------------------------------------------

static void print_interfere_map(Function func, Bitmap interfere) {
    printf("\nInterfere map\n");
    for(int i=0; i<func->all_vars->count; i++) {
        printf("%-10s ", SymbolList_get(func->all_vars, i)->name);
        for(int x=0; x<func->all_vars->count; x++) {
            if (bitmap_get(interfere, i, x))
                printf("X");
            else    
                printf(".");
            if (x%8==7)
                printf(" ");
        }
        printf("\n");
    }
}


// --------------------------------------------------------
//             replace_vars
// --------------------------------------------------------

static void replace_vars(Function func) {
    Instr i;
    foreach_instr(i, func) {
        if (i->dest && i->dest->kind==SYM_VARIABLE)
            i->dest = SymbolList_get(func->all_vars, i->dest->reg_number);
        if (i->a && i->a->kind==SYM_VARIABLE)
            i->a = SymbolList_get(func->all_vars, i->a->reg_number);
        if (i->b && i->b->kind==SYM_VARIABLE)
            i->b = SymbolList_get(func->all_vars, i->b->reg_number);
    }
}


// --------------------------------------------------------
//             register_allocation
// --------------------------------------------------------

void register_allocation(Function func) {
    Symbol sym;
    foreach_symbol(sym,func->all_vars)
        if (sym->kind==SYM_REG)
            sym->reg_number = sym->index;
        else
            sym->reg_number = UNALLOCATED;

    Bitmap interfere = build_interferemap(func);
    if (debug)
        print_interfere_map(func,interfere);

    attempt_combine_move(func,interfere);
    foreach_symbol(sym,func->all_vars)
        if (sym->kind==SYM_VARIABLE && sym->reg_number==UNALLOCATED) {
            int candidate = get_candidate_reg(interfere, sym->index);
            if (candidate==0) {
                // Eventually we should have spill logic to handle the case when register allocation fails
                // But for now simply abort with an error message
                error(0, "Register allocation failed for '%s'. Consider splitting into smaller functions", func->name);
            }

            allocate_register(func, interfere, SymbolList_get(func->all_vars, candidate), sym);
            attempt_combine_move(func,interfere);
        }

    replace_vars(func);
}
