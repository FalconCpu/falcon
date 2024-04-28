#include <stdio.h>
#include "fpl.h"
#include "AST.h"

// ---------------------------------------------------------------------------------
//                         AST_print
// ---------------------------------------------------------------------------------
// Print an AST for debug

void AST_print(AST this, int indent) {
    if (this==0)
        return;

    switch(this->kind) {
        case AST_INTLIT:    AST_intlit_print(as_intlit(this), indent); break;
        case AST_STRLIT:    AST_strlit_print(as_strlit(this), indent); break;
        case AST_SYMBOL:    AST_symbol_print(as_symbol(this), indent); break;
        case AST_BINOP:     AST_binop_print (as_binop (this), indent); break;
        case AST_UNARY:     AST_unary_print (as_unary (this), indent); break;
        case AST_INDEX:     AST_index_print (as_index (this), indent); break;
        case AST_MEMBER:    AST_member_print(as_member(this), indent); break;
        case AST_FUNCCALL:  AST_funccall_print(as_funccall(this), indent); break;
        case AST_DECL:      AST_decl_print  (as_decl  (this), indent); break;
        case AST_RETURN:    AST_return_print(as_return(this), indent); break;
        case AST_ASSIGN:    AST_assign_print(as_assign(this), indent); break;
        case AST_WHILE:     AST_while_print (as_while(this), indent); break;
        case AST_REPEAT:    AST_repeat_print(as_repeat(this), indent); break;
        case AST_IF:        AST_if_print    (as_if(this), indent); break;
        case AST_CLAUSE:    AST_clause_print(as_clause(this), indent); break;
        case AST_FUNCTION:  AST_function_print(as_function(this), indent); break;
        case AST_POINTER:   TODO("pointers");
        case AST_STRUCT:    AST_struct_print(as_struct(this), indent); break;
    }
}

// ---------------------------------------------------------------------------------
//                         AST_get_block
// ---------------------------------------------------------------------------------
// For kinds that contain a sub block - return it, or null if the kind does not have a block

Block AST_get_block(AST this) {
    switch(this->kind) {
        case AST_FUNCTION:  return as_function(this)->body;
        case AST_WHILE:     return as_while(this)->body;
        case AST_REPEAT:    return as_repeat(this)->body;
        case AST_IF:        return as_if(this)->clauses;
        case AST_CLAUSE:    return as_clause(this)->body;
        case AST_STRUCT:    return 0;

        case AST_INTLIT:    
        case AST_STRLIT:    
        case AST_SYMBOL:    
        case AST_BINOP:     
        case AST_UNARY:     
        case AST_INDEX:     
        case AST_MEMBER:    
        case AST_FUNCCALL:  
        case AST_DECL:      
        case AST_RETURN:   
        case AST_POINTER: 
        case AST_ASSIGN:    return 0;
    }
}


// ---------------------------------------------------------------------------------
//                           check_lvalue
// ---------------------------------------------------------------------------------
// Test to see if an expression is an lvalue

void check_lvalue(AST this) {
    switch(this->kind) {
        case AST_INDEX:
            // Array indexing is an lvalue
            return;

        case AST_MEMBER:
            // Struct members are lavalues
            return;
 
        case AST_POINTER:
            // Dereferenced pointers are also lvalues (can be assigned to)
            return;
 
        case AST_SYMBOL: {
            Symbol s = as_symbol(this)->symbol;
            if (s->kind!=SYM_VARIABLE && s->kind!=SYM_AGGREGATE && s->kind!=SYM_GLOBALVAR)
                error(this->location, "'%s' is not a variable", s->name);
            else if (s->mutable==0)
                error(this->location, "'%s' is not mutable", s->name);
            return;
        }
 
        default: error(this->location, "Not an lvalue");
    }
}

// ---------------------------------------------------------------------------------
//                         AST_get_constant_value
// ---------------------------------------------------------------------------------
// Determines if an AST has a known value, if so reports it
// if not give an error message and return zero
int AST_get_constant_value(AST this) {
    switch(this->kind) {
        case AST_SYMBOL: 
            if (as_symbol(this)->symbol->kind==SYM_CONSTANT)
                return as_symbol(this)->symbol->value.int_val;
            break;

        case AST_INTLIT:
            return as_intlit(this)->value;
        
        default: break;
    }

    error(this->location, "Expression is not a compile time constant");
    return 0;
}

// ---------------------------------------------------------------------------------
//                         AST_typecheck
// ---------------------------------------------------------------------------------
// runs the typechecking phase on any kind of AST node

void AST_typecheck(AST this, Block scope) {
    if (this==0)
        return;

    switch(this->kind) {
        case AST_INTLIT:    return AST_typecheck_intlit( as_intlit(this), scope);
        case AST_STRLIT:    return AST_typecheck_strlit( as_strlit(this), scope);
        case AST_SYMBOL:    return AST_typecheck_symbol( as_symbol(this), scope);
        case AST_ASSIGN:    return AST_typecheck_assign( as_assign(this), scope);
        case AST_INDEX:     return AST_typecheck_index(  as_index(this), scope);
        case AST_POINTER:   return AST_typecheck_pointer(as_pointer(this), scope);
        case AST_FUNCCALL:  return AST_typecheck_funccall(as_funccall(this), scope);
        case AST_WHILE:     return AST_typecheck_while(as_while(this), scope);
        case AST_REPEAT:    return AST_typecheck_repeat(as_repeat(this), scope);
        case AST_RETURN:    return AST_typecheck_return(as_return(this), scope);
        case AST_IF:        return AST_typecheck_if    (as_if(this), scope);
        case AST_CLAUSE:    return AST_typecheck_clause(as_clause(this), scope);
        case AST_MEMBER:    return AST_typecheck_member(as_member(this), scope);

        case AST_DECL:      return AST_typecheck_decl( as_decl(this), scope);
        case AST_FUNCTION:  return AST_typecheck_function( as_function(this), scope);
        case AST_BINOP:     return AST_typecheck_binop( as_binop(this), scope);
        case AST_UNARY:    return  AST_typecheck_unary( as_unary(this), scope);
        case AST_STRUCT:    return AST_typecheck_struct( as_struct(this), scope);
    }
}

