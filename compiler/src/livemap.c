#include <stdio.h>
#include "fpl.h"
#include "instr.h"
#include "types.h"

static Bitmap livemap;
static Bitmap killmap;

extern int debug;

/// -----------------------------------------------------------------
///                   set_read_write_bits
/// -----------------------------------------------------------------
/// For each instruction set a bit in the liveset if the instruction
/// reads from a variable, and a bit in the kill set if it writes to it

#define isvar(A) ((A)==SYM_VARIABLE || (A)==SYM_REG)

static void set_read_write_bits(Function func) {
    Instr i;
    foreach_instr(i,func) {
        switch(i->kind) {
            case INSTR_NOP:
            case INSTR_LABEL:
            case INSTR_JUMP:    break;

            case INSTR_LEA:
            case INSTR_MOV:     if (isvar(i->a->kind))
                                    bitmap_set(livemap, i->index, i->a->index);
                                if (isvar(i->dest->kind))
                                    bitmap_set(killmap, i->index, i->dest->index);
                                break;

            case INSTR_ALU:
            case INSTR_LOAD:    if (isvar(i->a->kind))
                                    bitmap_set(livemap, i->index, i->a->index);
                                if (isvar(i->b->kind))
                                    bitmap_set(livemap, i->index, i->b->index);
                                if (isvar(i->dest->kind))
                                    bitmap_set(killmap, i->index, i->dest->index);
                                break;

            case INSTR_STORE:   if (isvar(i->a->kind))
                                    bitmap_set(livemap, i->index, i->a->index);
                                if (isvar(i->b->kind))
                                    bitmap_set(livemap, i->index, i->b->index);
                                if (isvar(i->dest->kind))
                                    bitmap_set(livemap, i->index, i->dest->index);
                                break;

            case INSTR_CHK:
            case INSTR_BRANCH:  if (isvar(i->a->kind))
                                    bitmap_set(livemap, i->index, i->a->index);
                                if (isvar(i->b->kind))
                                    bitmap_set(livemap, i->index, i->b->index);
                                break;

            case INSTR_CALL:    for(int j=REG_ARGS; j<=REG_RESULT; j++)
                                    bitmap_set(killmap, i->index, j);
                                for(int j=REG_ARGS; j< REG_ARGS+  as_TypeFunction(i->a->type)->parameter_types->count + symbol_is_member_function(i->a); j++)
                                    bitmap_set(livemap, i->index, j);
                                break;

            case INSTR_START:   for(int j=0; j<func->params->count; j++)
                                    bitmap_set(livemap, i->index, REG_ARGS + j );
                                break;

            case INSTR_END:     if (func->return_type!=type_void)
                                    bitmap_set(livemap, i->index, REG_RESULT);
                                break;
        }
    }
}

/// -----------------------------------------------------------------
///                   propagate_liverange
/// -----------------------------------------------------------------
/// For each instruction propagate live information from any successor
/// instructions.
/// If the liveset at a label ever changes then set a flag to indicate
/// another iteration may be needed (as the label may be a target of a
/// jump or branch later in the code).

static int propagate_liverange(Function func) {
    int label_changed = 0;

    for(int index = func->num_instr-2; index>=0; index--) {
        Instr i = func->instr[index];
        switch(i->kind) {
            case INSTR_LABEL:
                label_changed |= bitmap_or_row(livemap, i->index, livemap, i->index+1);
                break;

            case INSTR_JUMP:
                label_changed |= bitmap_or_row(livemap, i->index, livemap, i->dest->index);
                break;

            case INSTR_BRANCH:
                label_changed |= bitmap_or_row(livemap, i->index, livemap, i->index+1);
                label_changed |= bitmap_or_row(livemap, i->index, livemap, i->dest->index);
                break;

            case INSTR_NOP:
            case INSTR_MOV:
            case INSTR_ALU:
            case INSTR_LOAD:
            case INSTR_STORE:
            case INSTR_CALL:
            case INSTR_LEA:
            case INSTR_CHK:
            case INSTR_START:
                bitmap_or_and_not_row(livemap, i->index, livemap, i->index+1, killmap, i->index);
                break;

            case INSTR_END:
                break;
        }
    }
    return label_changed;
}

/// -----------------------------------------------------------------
///                   gen_livemap
/// -----------------------------------------------------------------
/// Generate the livemap
/// A bitmap showing at each instruction and variable, weather the
/// variable may (along any path) have its present value read at any point in the future.
void gen_livemap(Function func) {
    if (killmap!=0)
        delete_Bitmap(killmap);
    if (func->livemap)
        delete_Bitmap(func->livemap);

    livemap = new_Bitmap(func->num_instr, func->all_vars->count);
    killmap = new_Bitmap(func->num_instr, func->all_vars->count);

    set_read_write_bits(func);

    while(propagate_liverange(func))
        ;

    if (debug) {
        printf("Livemap:\n");
        print_program_with_bitmap(func, livemap, killmap);
    }

    func->livemap = livemap;
}