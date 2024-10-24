#include "fpl.h"
#include <stdio.h>

// ====================================================
//                    Struct Definition
// ====================================================

struct AST_Function {
    Location location;
    int      kind;
    Type     type;

    String    name;
    AST_list  parameters;
    AST       ast_return_type;
    Block     body;
};


// ====================================================
//                    Constructor
// ====================================================

AST new_AST_Function(Location location, String name, AST_list parameters, AST return_type, Block body) {
    AST_Function ret = new(struct AST_Function);
    ret->location = location;
    ret->kind = AST_FUNCTION;
    ret->name = name;
    ret->parameters = parameters;
    ret->ast_return_type = return_type;
    ret->body = body;
    return (AST) ret;
}

// ====================================================
//                    Safe Cast
// ====================================================

AST_Function as_Function(AST n) {
    assert(n);
    if (n->kind!=AST_FUNCTION)
        fatal("Internal Error: Attempt to cast %s to Function   (Location %s.%d.%d)", token_kind_name(n->kind), n->location.filename, n->location.line, n->location.column);
    return (AST_Function) n;
}

// ====================================================
//                    Print
// ====================================================

void print_ast_Function(AST_Function n, int indent) {
    printf("Function %s\n", n->name);
    AST param;
    foreach(n->parameters, param)
        print_ast(param, indent+1);
    print_ast(n->ast_return_type, indent+1);
    print_block(n->body, indent+1);
}

// ====================================================
//              build_hierarchy_function
// ====================================================

void build_hierarchy_function(AST_Function this, Block context) {
    build_hierarchy_block(this->body, context);
    AST stmt;
    foreach_statement(this->body, stmt)
        build_hierarchy(stmt, this->body);
}


// ====================================================
//                    type check
// ====================================================

void type_check_Function(AST_Function n, Block context)  {
    TODO();
}
