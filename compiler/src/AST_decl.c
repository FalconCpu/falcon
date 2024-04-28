#include <stdio.h>
#include "fpl.h"
#include "AST.h"


// ============================================================================
//                          CONSTRUCTOR
// ============================================================================

AST new_AST_decl(Location location, String name, AST ast_type, AST ast_value, int mutable) {
    AST_decl ret = new(struct AST_decl);
    ret->location = location;
    ret->kind     = AST_DECL;
    ret->name     = name;
    ret->ast_type = ast_type;
    ret->ast_value= ast_value;
    ret->mutable  = mutable;
    return as_AST(ret);
}

// ============================================================================
//                          CAST
// ============================================================================

AST_decl as_decl(AST this) {
    assert(this->kind==AST_DECL);
    return (AST_decl) this;
}



// ============================================================================
//                         typeCheck
// ============================================================================
// Derive the type of the variable, generate a symbol for it and add to the symbol table

void AST_typecheck_decl(AST_decl this, Block scope) {
    AST_typecheck(this->ast_value, scope);

    Type type = (this->ast_type) ? resolve_AST_type(this->ast_type, scope)
              : (this->ast_value) ? this->ast_value->type
              : make_type_error(this->location, "Cannot determine type for '%s'", this->name);

    SymbolKind symk =   scope->parent==0 ? SYM_GLOBALVAR
                    :   is_scalar_type(type) ? SYM_VARIABLE : SYM_AGGREGATE;

    this->symbol = new_Symbol(this->location, symk, this->name, type, this->mutable);
    block_add_symbol(scope, this->symbol);

//    printf("decl %s %d %d %s\n",this->symbol->name, this->symbol->kind, this->symbol->offset, this->symbol->type->name);

    if (this->ast_value)
        check_type_compatible(type, this->ast_value);
}

// ============================================================================
//                          PRINT
// ============================================================================

void AST_decl_print(AST_decl this, int indent) {
    print_spaces(indent);
    printf("DECL %s %s\n", this->name, this->symbol ? this->symbol->type->name : "");
    AST_print(this->ast_type, indent+1);
    AST_print(this->ast_value, indent+1);
}

