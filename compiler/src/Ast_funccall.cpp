#include "fpl.h"

// ====================================================
//                   tree_print
// ====================================================

void Ast_funccall::tree_print(int indent) {
    std::cout << string(indent*2,' ') << "FUNCCALL " << std::endl;
    lhs->tree_print(indent+1);
    for(auto arg : args)
        arg->tree_print(indent+1);
}