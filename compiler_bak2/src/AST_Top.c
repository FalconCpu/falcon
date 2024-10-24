#include <stdio.h>
#include "fpl.h"

// ============================================
//                Struct definition
// ============================================

struct AST_Top {
    struct Block block;
};

// ============================================
//                Constructor
// ============================================

AST new_ast_top() {
    AST_Top ret = new(struct AST_Top);
    return (AST) ret;
}

// ============================================
//                Safe Cast
// ============================================

AST_IntLiteral as_Top(AST n) {

}


void           print_ast_Top(AST_Top n, int indent);
void           type_check_top(AST_Top n, Block context);

