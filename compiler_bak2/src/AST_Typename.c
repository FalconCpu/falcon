#include "fpl.h"
#include <stdio.h>

// ====================================================
//                    Struct Definition
// ====================================================

struct AST_TypeName {
    Location location;
    int      kind;
    Type     type;

    String   name;
};


// ====================================================
//                    Constructor
// ====================================================

AST new_ast_TypeName(Location location, String name) {
    AST_TypeName ret = new(struct AST_TypeName);
    ret->location = location;
    ret->kind = AST_TYPENAME;
    ret->name = name;
    return (AST) ret;
}

// ====================================================
//                    Safe Cast
// ====================================================

AST_TypeName as_TypeName(AST n) {
    assert(n);
    if (n->kind!=AST_TYPENAME)
        fatal("Internal Error: Attempt to cast %s to TypeName   (Location %s.%d.%d)", token_kind_name(n->kind), n->location.filename, n->location.line, n->location.column);
    return (AST_TypeName) n;
}

// ====================================================
//                    Print
// ====================================================

void print_ast_TypeName(AST_TypeName n, int indent) {
    printf("TypeName %s %s\n", n->name, n->type?n->type->name:"");
}

