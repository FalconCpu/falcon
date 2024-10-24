#include "fpl.h"

// ====================================================
//                   tree_print
// ====================================================

void Ast_member::tree_print(int indent) {
    std::cout << string(indent*2,' ') << "MEMBER " << *name << std::endl;
    lhs->tree_print(indent+1);
}