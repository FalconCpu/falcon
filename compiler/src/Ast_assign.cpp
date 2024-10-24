#include "fpl.h"

// ====================================================
//                   tree_print
// ====================================================

void Ast_assign::tree_print(int indent) {
    std::cout << string(indent*2,' ') << "ASSIGN" << std::endl;
    lhs->tree_print(indent+1);
    rhs->tree_print(indent+1);
}