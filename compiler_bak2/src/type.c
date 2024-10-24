#include <stdio.h>
#include "fpl.h"

Type type_unit;
Type type_null;
Type type_nothing;
Type type_bool;
Type type_char;
Type type_int;
Type type_real;
Type type_string;
Block predefined_names;

// ==========================================
//         new_type
// ==========================================

static Type new_type(int kind, String name) {
    Type ret = new(struct Type);
    ret->kind = kind;
    ret->name = name;

    hash_table_add(name, TOKEN_IDENTIFIER);
    block_add_type(predefined_names, ret);
    return ret;
}

// ==========================================
//         initialize_types
// ==========================================

void initialize_types() {
    predefined_names = new_block();

    type_unit = new_type(TYPE_UNIT, "unit");
    type_null = new_type(TYPE_NULL, "null");
    type_nothing = new_type(TYPE_NOTHING, "nothing");
    type_bool = new_type(TYPE_BOOL, "bool");
    type_char = new_type(TYPE_CHAR, "char");
    type_int = new_type(TYPE_INT, "int");
    type_real = new_type(TYPE_REAL, "real");
    type_string = new_type(TYPE_STRING, "string");
}

// ==========================================
//         get_type_from_code
// ==========================================

Type get_type_from_code(int code) {
    switch (code) {
        case TYPE_UNIT:    return type_unit;
        case TYPE_NULL:    return type_null;
        case TYPE_NOTHING: return type_nothing;
        case TYPE_BOOL:    return type_bool;
        case TYPE_CHAR:    return type_char;
        case TYPE_INT:     return type_int;
        case TYPE_REAL:    return type_real;
        case TYPE_STRING:  return type_string;
        default: fatal("Internal Error: Unknown type code %d", code);
    }
}
