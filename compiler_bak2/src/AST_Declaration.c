#include "fpl.h"
#include <stdio.h>

// ====================================================
//                    Struct Definition
// ====================================================

struct AST_Declaration {
    Location location;
    int      kind;
    Type     type;

    int      op;
    String   name;
    AST      ast_type;
    AST      value;
};

// ====================================================
//                    Constructor
// ====================================================

AST new_ast_Declaration(Location location, int op, String name, AST ast_type, AST value) {
    AST_Declaration ret = new(struct AST_Declaration);
    ret->location = location;
    ret->kind = AST_DECLARATION;
    ret->name = name;
    ret->op = op;   
    ret->ast_type = ast_type;
    ret->value = value;
    return (AST) ret;
}

// ====================================================
//                    Safe Cast
// ====================================================

AST_Declaration as_Declaration(AST n) {
    assert(n);
    if (n->kind!=AST_DECLARATION)
        fatal("Internal Error: Attempt to cast %s to Declaration   (Location %s.%d.%d)", token_kind_name(n->kind), n->location.filename, n->location.line, n->location.column);
    return (AST_Declaration) n;
}

// ====================================================
//                    Print
// ====================================================

void print_ast_Declaration(AST_Declaration n, int indent) {
    printf("DECL %s %s %s\n", token_kind_name(n->op), n->name, n->type?n->type->name:"");
    print_ast(n->ast_type, indent+1);
    print_ast(n->value, indent+1);
}

// ====================================================
//                    type check
// ====================================================

void type_check_Declaration(AST_Declaration n, Block context)  {
    TODO();
}