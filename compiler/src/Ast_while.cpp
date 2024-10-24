#include "fpl.h"

// ====================================================
//                   tree_print
// ====================================================

void Ast_while::tree_print(int indent) {
    std::cout << string(indent*2,' ') << "WHILE" << std::endl;
    condition->tree_print(indent+1);
    for(auto statement : statements)
        statement->tree_print(indent+1);
}