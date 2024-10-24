#include "fpl.h"

// ====================================================
//                   tree_print
// ====================================================

void Ast_declaration::tree_print(int indent) {
    std::cout << string(indent*2,' ') << "DECL " << *name << " " << get_token_name(op) << std::endl;
    if (type_expr)
        type_expr->tree_print(indent+1);
    if (init_val)
        init_val->tree_print(indent+1);
}