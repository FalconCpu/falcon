#include "fpl.h"
#include <stdio.h>

// ====================================================
//                    Struct Definition
// ====================================================

struct AST_Parameter {
    Location location;
    int      kind;
    Type     type;

    int      op;
    String   name;
    AST      ast_type;
};


// ====================================================
//                    Constructor
// ====================================================

AST new_ast_Parameter(Location location, int op, String name, AST ast_type) {
    AST_Parameter ret = new(struct AST_Parameter);
    ret->location = location;
    ret->kind = AST_PARAMETER;
    ret->op   = op;
    ret->name = name;
    ret->ast_type = ast_type;
    return (AST) ret;
}

// ====================================================
//                    Safe Cast
// ====================================================

AST_Parameter as_Parameter(AST n) {
    assert(n);
    if (n->kind!=AST_PARAMETER)
        fatal("Internal Error: Attempt to cast %s to Parameter   (Location %s.%d.%d)", token_kind_name(n->kind), n->location.filename, n->location.line, n->location.column);
    return (AST_Parameter) n;
}

// ====================================================
//                    Print
// ====================================================

void print_ast_Parameter(AST_Parameter n, int indent) {
    printf("PARAM %s %s\n", n->op ? token_kind_name(n->op) : "", n->name);
    print_ast(n->ast_type, indent+1);
}
