#include <stdio.h>
#include "fpl.h"
#include "AST.h"


// ============================================================================
//                          CONSTRUCTOR
// ============================================================================

AST new_AST_return(Location location, AST retval) {
    AST_return ret = new(struct AST_return);
    ret->location = location;
    ret->kind     = AST_RETURN;
    ret->retval   = retval;
    return as_AST(ret);
}

// ============================================================================
//                          CAST
// ============================================================================

AST_return as_return(AST this) {
    assert(this->kind==AST_RETURN);
    return (AST_return) this;
}


// ============================================================================
//                          PRINT
// ============================================================================

void AST_return_print(AST_return this, int indent) {
    print_spaces(indent);
    printf("RETURN\n");
    AST_print(this->retval, indent+1);
}

// ============================================================================
//                         typecheck
// ============================================================================

void AST_typecheck_return(AST_return this, Block scope) {
    if (this->retval)
        AST_typecheck_value(this->retval, scope);

    Function func = block_find_enclosing_function(scope);

    if (func==0)
        error(this->location, "return statement not inside a function");

    else if (func->return_type==type_void && this->retval!=0)
        error(this->location, "return with value in function returning Void");

    else if (func->return_type!=type_void && this->retval==0)
        error(this->location, "return should return %s",func->return_type->name);

    else if (func->return_type!=type_void && this->retval!=0)
        check_type_compatible(func->return_type, this->retval);
}

// ============================================================================
//                           code_gen
// ============================================================================

Symbol code_gen_return(Function func, AST_return this) {

    if (this->retval!=0) {
        Symbol retval = code_gen(func, this->retval);
        add_instr(func, new_Instr(INSTR_MOV, 0, SymbolList_get(func->all_vars,REG_RESULT), retval, 0));
    }
    add_instr(func, new_Instr(INSTR_JUMP, 0, func->end_label, 0, 0));
    return 0;
}
