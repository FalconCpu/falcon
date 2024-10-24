#include <iostream>
#include "fpl.h"

void Ast_identifier::tree_print(int indent) {
    std::cout << std::string(indent*2, ' ') << "ID " << *name << std::endl;
}