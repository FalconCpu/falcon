#include "fpl.h"

// ====================================================
//                   tree_print
// ====================================================

void Ast_binop::tree_print(int indent) {
    std::cout << string(indent*2,' ') << "BINOP " << get_token_name(op) << std::endl;
    lhs->tree_print(indent+1);
    rhs->tree_print(indent+1);
}