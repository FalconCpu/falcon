#include "fpl.h"

// ====================================================
//                   tree_print
// ====================================================

void Ast_unary::tree_print(int indent) {
    std::cout << string(indent*2,' ') << "UNARY " << get_token_name(op) << std::endl;
    arg->tree_print(indent+1);
}