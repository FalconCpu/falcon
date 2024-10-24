#include "fpl.h"
#include <stdio.h>

// ====================================================
//                    Struct Definition
// ====================================================

struct AST_Binop {
    Location location;
    int      kind;
    Type     type;

    int      op;
    AST      lhs;
    AST      rhs;
};

// ====================================================
//                    Constructor
// ====================================================

AST new_ast_Binop(Location location, int op, AST lhs, AST rhs) {
    AST_Binop ret = new(struct AST_Binop);
    ret->location = location;
    ret->kind = AST_BINOP;
    ret->op   = op;
    ret->lhs  = lhs;
    ret->rhs  = rhs;
    return (AST) ret;
}

// ====================================================
//                    Safe Cast
// ====================================================

AST_Binop as_Binop(AST n) {
    assert(n);
    if (n->kind!=AST_BINOP)
        fatal("Internal Error: Attempt to cast %s to Binop   (Location %s.%d.%d)", token_kind_name(n->kind), n->location.filename, n->location.line, n->location.column);
    return (AST_Binop) n;
}

// ====================================================
//                    Print
// ====================================================

void print_ast_Binop(AST_Binop n, int indent) {
    printf("Binop %s %s\n", token_kind_name(n->op), n->type?n->type->name:"");
    print_ast(n->lhs, indent+1);
    print_ast(n->rhs, indent+1);
}

// ====================================================
//                    type check
// ====================================================

void type_check_Binop(AST_Binop n, Block context)  {
    fatal("TODO");
}