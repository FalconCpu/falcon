#include <stdio.h>
#include "fpl.h"
#include "AST.h"
#include "instr.h"

typedef struct ASTnode_cast*     ASTnode_cast;

// ---------------------------------------------------------------------------------
//                         Constructor
// ---------------------------------------------------------------------------------

AST new_AST_cast(Location loc, AST expr, AST type_expr) {
    AST_cast ret = new(struct AST_cast);
    ret->location = loc;
    ret->kind = AST_CAST;
    ret->expr = expr;
    ret->type_expr = type_expr;
    return (AST)ret;
}

// ---------------------------------------------------------------------------------
//                         Print
// ---------------------------------------------------------------------------------

void AST_cast_print(AST_cast node, int indent) {
    print_spaces(indent);
    printf("CAST %s\n", node->type ? node->type->name : "");
    AST_print(node->expr, indent+1);
}

// ---------------------------------------------------------------------------------
//                         asnullchk
// ---------------------------------------------------------------------------------

AST_cast as_cast(AST this) {
    if (this==0)
        fatal("Null cast to ASTnode_cast");
    if (this->kind!=AST_CAST)
        fatal("Attempt to cast %x to cast", this->kind);
    return (AST_cast) this;
}

// ---------------------------------------------------------------------------------
//                         typecheck
// ---------------------------------------------------------------------------------

void AST_typecheck_cast(AST_cast this, Block scope) {
    AST_typecheck_value(this->expr, scope);
    Type to_type = resolve_AST_type(this->type_expr, scope);
    
    if (this->expr->type==type_error) {
        this->type = type_error;
        return;
    }
    
    if (this->expr->type == to_type) {
        warn(this->location, "Redundant cast");
    }


    if (!is_scalar_type(this->expr->type) || !is_scalar_type(to_type)) {
        this->type = make_type_error(this->location, "Cannot cast '%s' to '%s'", this->expr->type->name, to_type->name);
        return;
    }

    this->type = to_type;
}

// ---------------------------------------------------------------------------------
//                           code_gen_index
// ---------------------------------------------------------------------------------

Symbol code_gen_cast(Function func, AST_cast this) {
    Symbol lhs = code_gen(func, this->expr);

    if (lhs->type==type_real || this->type==type_real)
        TODO("Casts to/from real numbers not yet implemented");

    
    Symbol ret = new_tempvar(func, this->type);
    add_instr(func, new_Instr(INSTR_MOV, 0, ret, lhs, 0));
    return ret;
}

