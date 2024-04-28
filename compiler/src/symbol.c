#include <stdio.h>
#include <string.h>
#include "fpl.h"

// ---------------------------------------------------------------------------------
//                         Symbols
// ---------------------------------------------------------------------------------
// Symbols are used to represent anything that can be searched for by name
// primarily variables, but also constants, functions, and types.

Symbol new_Symbol(Location loc, int kind, String name, Type type, int mutable) {
    Symbol ret = new(struct Symbol);
    if (loc)
        ret->location = loc;
    ret->kind = kind;
    ret->name = name;
    ret->type = type;
    ret->mutable = mutable;
    return ret;
}

// ---------------------------------------------------------------------------------
//                         symbol_kind_name
// ---------------------------------------------------------------------------------
// Array of symbol kind descriptions, mostly for use in error messages

String symbol_kind_name[] = {
    "constant",
    "variable",
    "function",
    "type descriptor",
    "aggregate",
    "label",
    "register"
};

// ---------------------------------------------------------------------------------
//                         Symbol_toText
// ---------------------------------------------------------------------------------

String Symbol_toText(Symbol this) {
    char buf[64];
    char* k = this->kind==SYM_CONSTANT ? "CONST"
            : this->kind==SYM_VARIABLE ? "VAR"
            : this->kind==SYM_TYPEREF  ? "TYPE"
            : this->kind==SYM_FUNCTION ? "FUNC"
            : this->kind==SYM_AGGREGATE? "AGGREGATE"
            : this->kind==SYM_GLOBALVAR? "GLOBAL"
            :                            "ERROR";

    sprintf(buf,"%s:%s:%s ",this->name, k, this->type ? this->type->name : "null");
    return strdup(buf);
}

// ---------------------------------------------------------------------------------
//                         get_aggregate_address
// ---------------------------------------------------------------------------------
// Generate code to find the address of a symbol

// Symbol get_aggregate_address(Function func, Symbol this) {
//     if (this->kind==SYM_AGGREGATE) {
//         // We have an variable on our local stack
//         Symbol ret = new_tempvar(func, this->type);
//         Symbol sp = SymbolList_get(func->all_vars,REG_SP);
//         Symbol offset = make_constant_symbol(func, this->offset);
//         add_instr(func, new_Instr(INSTR_ALU, ALU_ADD_I, ret, sp, offset));
//         return ret;
//     } else {
//         // We have an aggregate passed into us
//         return this;
//     }
// }

// -------------------------------------------------------------------------------
// Structure to represent a list of symbols
// -------------------------------------------------------------------------------

SymbolList new_SymbolList() {
    SymbolList ret = new(struct SymbolList);
    ret->alloc = 8;
    ret->symbols = new_array(Symbol, ret->alloc);
    return ret;
}

void SymbolList_add(SymbolList list, Symbol item) {
    if (list->count == list->alloc) {
        list->alloc *= 2;
        resize_array(list->symbols, Symbol, list->alloc);
    }
    list->symbols[list->count++] = item;
}

Symbol SymbolList_get(SymbolList list, int index) {
    assert(list);
    if (index<0 || index>list->count)
        fatal("Index %d out of bounds in TypeList_get",index);
    if (index==list->count)
        return 0;
    else
        return list->symbols[index];
}

Symbol SymbolList_find(SymbolList list, String name) {
    // Look for a symbol in the list with the given name, if found return it
    // if not return 0

    // Because all strings are stored as links to the entry in the lexer's hash
    // table - we can guarantee that equal strings will also have equal pointer values.
    // Therefore we can compare pointers, rather than needing to call strcmp to compare
    // strings

    Symbol ret;
    foreach_symbol(ret, list)
        if (ret->name==name)
            return ret;
    return 0;
}