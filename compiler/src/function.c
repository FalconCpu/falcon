#include <stdio.h>
#include "fpl.h"
#include "AST.h"


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

