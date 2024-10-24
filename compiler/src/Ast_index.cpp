#include "fpl.h"

// ====================================================
//                   tree_print
// ====================================================

void Ast_index::tree_print(int indent) {
    std::cout << string(indent*2,' ') << "INDEX" << std::endl;
    lhs->tree_print(indent+1);
    rhs->tree_print(indent+1);
}