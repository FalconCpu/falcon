#include "fpl.h"
#include "ast.h"
#include <stdio.h>

// ====================================================
//                    Constructor
// ====================================================

AST new_ast_IntLiteral(Location location, int value) {
    AST_IntLiteral ret = new(struct AST_IntLiteral);
    ret->location = location;
    ret->kind = AST_INT_LITERAL;
    ret->value = value;
    return (AST) ret;
}

// ====================================================
//                    StringToInt
// ====================================================

int stringToInt(String s) {
    int ret = 0;
    for(int k=0; s[k]; k++)
        ret = ret*10 + (s[k]-'0');
    return ret;
}


// ====================================================
//                    Safe Cast
// ====================================================

AST_IntLiteral as_IntLiteral(AST n) {
    assert(n);
    if (n->kind!=AST_INT_LITERAL)
        fatal("Internal Error: Attempt to cast %s to IntLiteral   (Location %s.%d.%d)", token_kind_name(n->kind), n->location.filename, n->location.line, n->location.column);
    return (AST_IntLiteral) n;
}

// ====================================================
//                    Print
// ====================================================

void print_ast_IntLiteral(AST_IntLiteral n, int indent) {
    printf("IntLiteral %d %s\n", n->value, n->type?n->type->name:"");
}

