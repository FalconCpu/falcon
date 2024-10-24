#include "fpl.h"
#include <stdio.h>

// ====================================================
//                    Struct definition
// ====================================================

struct AST_Identifier {
    Location location;
    int      kind;
    Type     type;

    String   name;
};

// ====================================================
//                    Constructor
// ====================================================

AST new_ast_Identifier(Location location, String name) {
    AST_Identifier ret = new(struct AST_Identifier);
    ret->location = location;
    ret->kind = AST_IDENTIFIER;
    ret->name = name;
    return (AST) ret;
}

// ====================================================
//                    Safe Cast
// ====================================================

AST_Identifier as_Identifier(AST n) {
    assert(n);
    if (n->kind!=AST_IDENTIFIER)
        fatal("Internal Error: Attempt to cast %s to Identifier   (Location %s.%d.%d)", token_kind_name(n->kind), n->location.filename, n->location.line, n->location.column);
    return (AST_Identifier) n;
}

// ====================================================
//                    Print
// ====================================================

void print_ast_Identifier(AST_Identifier n, int indent) {
    printf("ID %s %s\n", n->name, n->type?n->type->name:"");
}

// ====================================================
//                    TypeCheck
// ====================================================

void type_check_Identifier(AST_Identifier n, Block context) {
    fatal("TODO");
}