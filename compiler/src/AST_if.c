#include <stdio.h>
#include "fpl.h"
#include "AST.h"


// ============================================================================
//                          CONSTRUCTOR
// ============================================================================

AST new_AST_if(Location location, Block clauses) {
    AST_if ret = new(struct AST_if);
    ret->location = location;
    ret->kind     = AST_IF;
    ret->clauses  = clauses;
    return as_AST(ret);
}

// ============================================================================
//                          CAST
// ============================================================================

AST_if as_if(AST this) {
    assert(this->kind==AST_IF);
    return (AST_if) this;
}


// ============================================================================
//                          PRINT
// ============================================================================

void AST_if_print(AST_if this, int indent) {
    print_spaces(indent);
    printf("IF\n");
    AST clause;
    foreach_statement(clause, this->clauses)
        AST_print(clause, indent+1);
}


// ============================================================================
//                         typecheck
// ============================================================================

void AST_typecheck_if(AST_if this, Block scope) {
    (void) scope;
    block_typecheck(this->clauses);
}

// ---------------------------------------------------------------------------------
//                           code_gen
// ---------------------------------------------------------------------------------

Symbol code_gen_if(Function func, AST_if this) {
    Symbol lab_next = new_label(func);
    Symbol lab_end = new_label(func);

    AST clause;
    foreach_statement(clause, this->clauses) {
        code_gen_clause(func, as_clause(clause), lab_next, lab_end);
        add_instr(func, new_Instr(INSTR_LABEL , 0, lab_next, 0,0));
        lab_next = new_label(func);
    }
    add_instr(func, new_Instr(INSTR_LABEL , 0, lab_end, 0,0));
    return 0;
}
