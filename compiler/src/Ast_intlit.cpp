#include <string>
#include "fpl.h"

Ast_intlit::Ast_intlit(Location location, string* name) : Ast_expression(location) {
    try {
        size_t num_chars_converted = 0;
        value = std::stoi(*name, &num_chars_converted, 0);
        if (num_chars_converted != name->length()) {
            error(location, "Invalid integer literal: " + *name);
            value = 0;
        }
    } catch (const std::invalid_argument&) {
        error(location, "Invalid integer literal: " + *name);
        value = 0;
    } catch (const std::out_of_range&) {
        error(location, "Integer literal out of range: " + *name);
        value = 0;
    }
}

void Ast_intlit::tree_print(int indent) {
    std::cout << string(indent*2,' ') << "INTLIT " << value << std::endl;
}
