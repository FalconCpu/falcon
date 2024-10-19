#include <stdio.h>
#include "fpl.h"
#include "AST.h"


// ============================================================================
//                          CONSTRUCTOR
// ============================================================================

AST new_AST_clause(Location location, AST condition, Block body) {
    AST_clause ret = new(struct AST_clause);
    ret->location = location;
    ret->kind     = AST_CLAUSE;
    ret->condition= condition;
    ret->body     = body;
    return as_AST(ret);
}

// ============================================================================
//                          CAST
// ============================================================================

AST_clause as_clause(AST this) {
    assert(this->kind==AST_CLAUSE);
    return (AST_clause) this;
}


// ============================================================================
//                          PRINT
// ============================================================================

void AST_clause_print(AST_clause this, int indent) {
    print_spaces(indent);
    printf("CLAUSE\n");
    AST_print(this->condition, indent+1);
    block_print(this->body, indent+1);
}

// ============================================================================
//                         typecheck
// ============================================================================

void AST_typecheck_clause(AST_clause this, Block scope) {
    if (this->condition) {
        AST_typecheck(this->condition, scope);
        check_type_compatible(type_bool, this->condition);
    }
    block_typecheck(this->body);
}

// ============================================================================
//                           code_gen
// ============================================================================

Symbol code_gen_clause(Function func, AST_clause this, Symbol lab_next, Symbol lab_end) {
    if (this->condition) {
        Symbol lab_body = new_label(func);
        code_gen_bool(func, this->condition, lab_body, lab_next);
        add_instr(func, new_Instr(INSTR_LABEL , 0, lab_body, 0,0));
        code_gen_block(func, this->body);
        add_instr(func, new_Instr(INSTR_JUMP , 0, lab_end, 0,0));
    } else {
        code_gen_block(func, this->body);
        add_instr(func, new_Instr(INSTR_JUMP , 0, lab_end, 0,0));
    }
 
    return 0;
}
