#include <string.h>
#include <stdio.h>
#include "fpl.h"
#include "AST.h"
#include "types.h"


// ============================================================================
//                          All Functions
// ============================================================================
// keep a list of all functions with definitions
// (External functions are not included in this list)

static int num_functions = 0;
static int alloc_functions = 0;
Function* all_functions;

Function get_function_by_index(int index) {
    if (index>=num_functions)
        return 0;
    return all_functions[index];
}

// ============================================================================
//                         add_machine_regs
// ============================================================================
// Create models of the cpu registers for use in the TAC code

static void add_machine_regs(Function this) {
    SymbolList_add(this->all_vars, new_Symbol(0, SYM_REG, "0", type_int, 0));  // Register 0 is hardwired to zero
    for(int k=1; k<=28; k++) {
        char buf[8];
        sprintf(buf,"$%d",k);
        SymbolList_add(this->all_vars, new_Symbol(0, SYM_REG, strdup(buf), type_int, 0));
    }
    SymbolList_add(this->all_vars, new_Symbol(0, SYM_REG, "$gp", type_int, 0));    // globals pointer
    SymbolList_add(this->all_vars, new_Symbol(0, SYM_REG, "$ra", type_int, 0));    // return address
    SymbolList_add(this->all_vars, new_Symbol(0, SYM_REG, "$sp", type_int, 0));    // stack pointer
}


// ============================================================================
//                          CONSTRUCTOR
// ============================================================================

AST new_function(Location location, String name, AST_list params, AST ret_type_ast, Block body) {
    Function ret = new(struct Function);
    ret->location = location;
    ret->kind     = AST_FUNCTION;
    ret->name     = name;
    ret->params   = params;
    ret->ret_type_ast = ret_type_ast;
    ret->body     = body;

    ret->all_labels = new_SymbolList();
    ret->all_vars   = new_SymbolList();
    ret->all_consts = new_SymbolList();

    ret->alloc_instr = 64;
    ret->instr       = new_array(Instr, ret->alloc_instr);

    if (body!=0) {
        if (alloc_functions==0) {
            alloc_functions = 8;
            all_functions = new_array(Function, alloc_functions);
        } else if (num_functions==alloc_functions) {
            alloc_functions *= 2;
            resize_array(all_functions, Function, alloc_functions);
        }
        all_functions[num_functions++] = ret;
    }
    add_machine_regs(ret);
    return as_AST(ret);
}

// ============================================================================
//                          CAST
// ============================================================================

Function as_function(AST this) {
    assert(this->kind==AST_FUNCTION);
    return (Function) this;
}


// ============================================================================
//                         typecheck
// ============================================================================

void AST_typecheck_function(Function this, Block scope) {
    // Nothing much to do here - the parameters were already checked before
    // we get to this stage
    (void) scope;
    if (this->body==0)
        return; // Got an 'extern' function

    block_typecheck(this->body);
}


// ============================================================================
//                          PRINT
// ============================================================================

void AST_function_print(Function this, int indent) {
    print_spaces(indent);
    printf("FUNCTION %s\n", this->name);
    AST param;
    foreach(param, this->params)
        AST_print(param, indent+1);
    block_print(this->body, indent+1);
}

// ---------------------------------------------------------------------------------
//                         get_constant_symbol
// ---------------------------------------------------------------------------------
// Find a symbol relating to a given constant
Symbol make_constant_symbol(Function this, int value) {
    Symbol s;
    foreach_symbol(s, this->all_consts)
        if (s->type==type_int && s->value.int_val==value)
            return s;

    char buf[16];
    sprintf(buf,"%d", value);

    s = new_Symbol(0, SYM_CONSTANT, strdup(buf), type_int, 0);
    s->value.int_val = value;
    SymbolList_add(this->all_consts, s);
    return s;
}

// ---------------------------------------------------------------------------------
//                         get_string_constant_symbol
// ---------------------------------------------------------------------------------
// Find a symbol relating to a given constant

Symbol make_string_constant_symbol(Function this, String value) {
    Symbol s;
    foreach_symbol(s, this->all_consts)
        if (s->type==type_string && s->value.str_val==value)
            return s;

    s = new_Symbol(0, SYM_CONSTANT, value, type_string, 0);
    s->value.str_val = value;
    SymbolList_add(this->all_consts, s);
    return s;
}


// ---------------------------------------------------------------------------------
//                         get_codegen_symbol
// ---------------------------------------------------------------------------------
// check the given symbol is in the post codegen symbol table
// If the symbol is SYM_AGGREGATE then allocate stack spce for it
Symbol get_codegen_symbol(Function this, Symbol sym) {
    Symbol s;
    foreach_symbol(s, this->all_vars)
        if (s==sym)
            return sym;

    if (sym->kind==SYM_AGGREGATE) {
        if (sym->type->kind==TYPE_ARRAY)
            this->stack_vars+=4; // allow space for the array max index

        sym->offset = this->stack_vars;
        this->stack_vars += get_sizeof(sym->type);
    }

    SymbolList_add(this->all_vars, sym);
    return sym;
}

// ---------------------------------------------------------------------------------
//                         new_tempvar
// ---------------------------------------------------------------------------------
// create a new tempvar symbol
Symbol new_tempvar(Function this, Type type) {
    char buf[16];
    sprintf(buf, "#%d", this->num_temps ++);

    Symbol ret = new_Symbol(0, SYM_VARIABLE, strdup(buf), type, 1);
    SymbolList_add(this->all_vars, ret);
    return ret;
}

// ---------------------------------------------------------------------------------
//                         new_label
// ---------------------------------------------------------------------------------
// create a new tempvar symbol
Symbol new_label(Function this) {
    char buf[16];
    sprintf(buf, "@%d", this->all_labels->count);
    Symbol ret = new_Symbol(0, SYM_LABEL, strdup(buf), 0, 0);
    SymbolList_add(this->all_labels, ret);
    return ret;
}


// ---------------------------------------------------------------------------------
//                         add_instr
// ---------------------------------------------------------------------------------
// add an instruction to the program

void add_instr(Function this, Instr inst) {
    if (this->num_instr == this->alloc_instr) {
        this->alloc_instr *= 2;
        resize_array(this->instr, Instr, this->alloc_instr);
    }
    inst->index = this->num_instr;
    this->instr[this->num_instr++] = inst;
}



// ---------------------------------------------------------------------------------
//                         print_program
// ---------------------------------------------------------------------------------

void print_program(Function this) {
    Instr i;
    printf("\n%s:\n", this->name);
    foreach_instr(i, this)
        printf("%s\n", instr_toString(i));

}

// ---------------------------------------------------------------------------------
//                         print_header_row
// ---------------------------------------------------------------------------------

static void print_header_row(Function func, int row) {
    printf("                               ");
    for(int col=0; col<func->all_vars->count; col++) {
        String name = func->all_vars->symbols[col]->name;
        int index = strlen(name) -1 - row;
        if (index<0)
            printf(" ");
        else
            printf("%c",name[index]);
        if (col%4==3)
            printf(" ");
    }
    printf("\n");
}


// ---------------------------------------------------------------------------------
//                         print_program_with_bitmap
// ---------------------------------------------------------------------------------

void print_program_with_bitmap(Function this, Bitmap bm1, Bitmap bm2) {
    Instr i;
    printf("\n%s:\n", this->name);
    for(int k=6; k>=0; k--)
        print_header_row(this,k);
    foreach_instr(i, this) {
        String str = instr_toString(i);
        printf("%-30s ", str);

        // if (index==10) {
        //     for (int x=0; x<strlen(str); x++)
        //         printf("X: %d %d %c\n", x, str[x], str[x]);
        // }

        for(int col=0; col<bm1->num_cols; col++) {
            int bit= bitmap_get(bm1, i->index, col) + 2*bitmap_get(bm2, i->index, col);
            printf("%c", ".*KB"[bit]);
        if (col%4==3)
            printf(" ");
        }
        printf("\n");
    }
}


// ---------------------------------------------------------------------------------
//                           code_gen
// ---------------------------------------------------------------------------------

Symbol code_gen_function(Function this) {
    add_instr(this, new_Instr(INSTR_START, 0, 0, 0, 0));
    this->end_label = new_label(this);

    // give the function a qualified name if it is a member function
    if (this->this_sym) {
        this->name = my_strcat3(as_TypePointer(this->this_sym->type)->base->name,".", this->name);
        this->sym->name = this->name;
    }

    // get the functions parameters
    int param_no = 1;
    if (this->this_sym!=0)
        add_instr(this, new_Instr(INSTR_MOV, 0, get_codegen_symbol(this,this->this_sym), SymbolList_get(this->all_vars, param_no++), 0));
    Symbol s;
    foreach_symbol(s, this->parameters)
        add_instr(this, new_Instr(INSTR_MOV, 0, s, SymbolList_get(this->all_vars, param_no++), 0));

    // generate code for body of the function
    code_gen_block(this, this->body);

    // and end
    add_instr(this, new_Instr(INSTR_LABEL, 0, this->end_label, 0, 0));
    add_instr(this, new_Instr(INSTR_END, 0, 0, 0, 0));
    return 0;
}

