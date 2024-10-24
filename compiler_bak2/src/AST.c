#include "fpl.h"
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
         case AST_DECLARATION: print_ast_Declaration(as_Declaration(n), indent); break;
         case AST_TYPENAME:    print_ast_TypeName(as_TypeName(n), indent); break;
         case AST_PARAMETER:   print_ast_Parameter(as_Parameter(n), indent); break;
         case AST_FUNCTION:    print_ast_Function(as_Function(n), indent); break;
         default:              fatal("Internal Error: Unknown AST node in print_ast: kind %d\n", n->kind);
    }
}

// ====================================================
//                    build_hierarchy
// ====================================================

void build_hierarchy(AST this, Block context) {
    switch(this->kind) {
        case AST_FUNCTION:    build_hierarchy_function( as_Function(this), context); break;
        case AST_INT_LITERAL: 
        case AST_IDENTIFIER:  
        case AST_BINOP:       
        case AST_DECLARATION: 
        case AST_TYPENAME:    
        case AST_PARAMETER:   break; // Do nothing 
        default:              fatal("Internal Error: Invalid AST node in build_hierarchy: kind %d\n", this->kind);
    }
    
}

// ====================================================
//                    TypeCheck
// ====================================================

void type_check(AST n, Block context) {
    switch(n->kind) {
        case AST_INT_LITERAL: type_check_IntLiteral(as_IntLiteral(n), context); break;
        case AST_IDENTIFIER:  type_check_Identifier(as_Identifier(n), context); break;
        case AST_BINOP:       type_check_Binop(as_Binop(n), context); break;
        case AST_DECLARATION: type_check_Declaration(as_Declaration(n), context); break;
        case AST_FUNCTION:    type_check_Function(as_Function(n), context); break;
        case AST_TYPENAME:    
        case AST_PARAMETER:   fatal("Internal Error: Invalid AST node in type_check: kind %d\n", n->kind);
        default:              fatal("Internal Error: Unknown AST node in type_check: kind %d\n", n->kind);
    }
}
