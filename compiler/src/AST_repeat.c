#include <stdio.h>
#include "fpl.h"
#include "AST.h"


// ============================================================================
//                          CONSTRUCTOR
// ============================================================================

AST new_AST_repeat(Location location, AST condition, Block body) {
    AST_repeat ret = new(struct AST_repeat);
    ret->location = location;
    ret->kind     = AST_REPEAT;
    ret->condition= condition;
    ret->body     = body;
    return as_AST(ret);
}

// ============================================================================
//                          CAST
// ============================================================================

AST_repeat as_repeat(AST this) {
    assert(this->kind==AST_REPEAT);
    return (AST_repeat) this;
}


// ============================================================================
//                          PRINT
// ============================================================================

void AST_repeat_print(AST_repeat this, int indent) {
    print_spaces(indent);
    printf("REPEAT\n");
    AST_print(this->condition, indent+1);
    block_print(this->body, indent+1);
}

// ============================================================================
//                         typecheck
// ============================================================================

void AST_typecheck_repeat(AST_repeat this, Block scope) {
    AST_typecheck_value(this->condition, scope);
    check_type_compatible(type_bool, this->condition);
    block_typecheck(this->body);
}

// ============================================================================
//                           code_gen
// ============================================================================

Symbol code_gen_repeat(Function func, AST_repeat this) {
    Symbol lab_start = new_label(func);
    Symbol lab_cond = new_label(func);
    Symbol lab_end = new_label(func);

    add_instr(func, new_Instr(INSTR_LABEL , 0, lab_start, 0,0));
    code_gen_block(func, this->body);
    add_instr(func, new_Instr(INSTR_LABEL , 0, lab_cond, 0,0));
    code_gen_bool(func, this->condition, lab_start, lab_end);
    add_instr(func, new_Instr(INSTR_LABEL , 0, lab_end, 0,0));
    return 0;
}