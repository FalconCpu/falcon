#include <iostream>
#include "fpl.h"

// ============================================================
//                       tree_print
// ============================================================

void Ast_block :: tree_print(int indent) {
    std::cout << string(indent*2,' ') << "block" << std::endl;
    for (auto statement : statements) {
        statement->tree_print(indent + 1);
    }
}

// ============================================================
//                       add
// ============================================================

void Ast_block :: add(Ast* statement) {
    statements.push_back(statement);
    if (Ast_block* block = dynamic_cast<Ast_block*>(statement))
        block->parent = this;
}

