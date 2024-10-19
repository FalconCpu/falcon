#include "fpl.h"
#include "ast.h"
#include <stdio.h>

// ====================================================
//                    Print
// ====================================================

void print_ast(AST n, int indent) {
    if (n==0)
        return;

    for(int k=0; k<indent; k++)
        printf("  ");

    switch(n->kind) {
         case AST_INT_LITERAL: print_ast_IntLiteral(as_IntLiteral(n), indent); break;
         case AST_IDENTIFIER:  print_ast_Identifier(as_Identifier(n), indent); break;
         case AST_BINOP:       print_ast_Binop(as_Binop(n), indent); break;
         default: fatal("Internal Error: Unknown AST node in print_ast: kind %d\n", n->kind);
    }
}
