#include <stdio.h>
#include "fpl.h"
#include "AST.h"
#include "types.h"

// ============================================================================
//                          CONSTRUCTOR
// ============================================================================

AST new_AST_struct(Location location,String name, AST_list members, Block body) {
    AST_struct ret = new(struct AST_struct);
    ret->location = location;
    ret->kind     = AST_STRUCT;
    ret->name     = name;
    ret->members  = members;
    ret->body     = body;
    return as_AST(ret);
}

// ============================================================================
//                          CAST
// ============================================================================

AST_struct as_struct(AST this) {
    assert(this->kind==AST_STRUCT);
    return (AST_struct) this;
}


// ============================================================================
//                          PRINT
// ============================================================================

void AST_struct_print(AST_struct this, int indent) {
    print_spaces(indent);
    printf("STRUCT %s\n", this->name);
    AST member;
    foreach(member, this->members)
        AST_print(member, indent+1);
    block_print(this->body, indent+1);
}

// ---------------------------------------------------------------------------------
//                         typecheck
// ---------------------------------------------------------------------------------

void AST_typecheck_struct(AST_struct this, Block scope) {
    // Resolving the types of structure members is done during the "identify_functions"
    // phase of semantic checking.
    // However during the typecheck phase we calculate the offsets of all struct members
    (void) scope;
    
    int offset =0;
    struct Type_struct *type = (struct Type_struct *) this->type;
    Symbol sym;

    foreach_symbol(sym, type->body->symbols) {
        if (sym->kind!=SYM_FIELD)   
            continue;
        int size = get_sizeof(sym->type);
        // add padding if needed
        if (size==2)
            offset = ((offset-1) | 1) +1;
        else if (size==4)
            offset = ((offset-1) | 3) +1;
        sym->offset = offset;
        offset += get_sizeof(sym->type);
    }
    type->size = offset;

    block_typecheck(this->body);
}

