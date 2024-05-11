#include <stdio.h>
#include "fpl.h"
#include "AST.h"
#include "instr.h"

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
        case AST_CAST:      AST_cast_print  (as_cast (this), indent); break;
        case AST_INDEX:     AST_index_print (as_index (this), indent); break;
        case AST_MEMBER:    AST_member_print(as_member(this), indent); break;
        case AST_NEW:       AST_new_print   (as_new(this), indent); break;
        case AST_FUNCCALL:  AST_funccall_print(as_funccall(this), indent); break;
        case AST_DECL:      AST_decl_print  (as_decl  (this), indent); break;
        case AST_RETURN:    AST_return_print(as_return(this), indent); break;
        case AST_ASSIGN:    AST_assign_print(as_assign(this), indent); break;
        case AST_ASSIGNOP:  AST_assignOp_print(as_assignOp(this), indent); break;
        case AST_WHILE:     AST_while_print (as_while(this), indent); break;
        case AST_REPEAT:    AST_repeat_print(as_repeat(this), indent); break;
        case AST_IF:        AST_if_print    (as_if(this), indent); break;
        case AST_CLAUSE:    AST_clause_print(as_clause(this), indent); break;
        case AST_FUNCTION:  AST_function_print(as_function(this), indent); break;
        case AST_POINTER:   TODO("pointers");
        case AST_STRUCT:    AST_struct_print(as_struct(this), indent); break;
        case AST_FOR:       AST_for_print(as_for(this), indent); break;
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
        case AST_FOR:       return as_for(this)->body;
        case AST_STRUCT:    return as_struct(this)->body;

        case AST_INTLIT:    
        case AST_STRLIT:    
        case AST_SYMBOL:    
        case AST_NEW:    
        case AST_BINOP:     
        case AST_CAST:     
        case AST_UNARY:     
        case AST_INDEX:     
        case AST_MEMBER:    
        case AST_FUNCCALL:  
        case AST_DECL:      
        case AST_RETURN:   
        case AST_POINTER: 
        case AST_ASSIGNOP:
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
        case AST_ASSIGNOP:  return AST_typecheck_assignOp( as_assignOp(this), scope);
        case AST_INDEX:     return AST_typecheck_index(  as_index(this), scope);
        case AST_CAST:      return AST_typecheck_cast(  as_cast(this), scope);
        case AST_NEW:       return AST_typecheck_new(   as_new(this), scope);
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
        case AST_FOR:       return AST_typecheck_for( as_for(this), scope);
    }
}

// ---------------------------------------------------------------------------------
//                         code_gen
// ---------------------------------------------------------------------------------
// Generate IR code

Symbol code_gen(Function func, AST this) {
    switch(this->kind) {
        case AST_INTLIT:    return code_gen_intlit(func, as_intlit(this));
        case AST_STRLIT:    return code_gen_strlit(func, as_strlit(this));
        case AST_SYMBOL:    return code_gen_symbol(func, as_symbol(this));
        case AST_ASSIGN:    return code_gen_assign(func, as_assign(this));
        case AST_ASSIGNOP:  return code_gen_assignOp(func, as_assignOp(this));
        case AST_INDEX:     return code_gen_index(func, as_index(this));
        case AST_CAST:      return code_gen_cast(func, as_cast(this));
        case AST_NEW:       return code_gen_new(func, as_new(this));
        case AST_POINTER:   return code_gen_pointer(func, as_pointer(this));
        case AST_FUNCCALL:  return code_gen_funccall(func, as_funccall(this));
        case AST_WHILE:     return code_gen_while(func, as_while(this));
        case AST_REPEAT:    return code_gen_repeat(func, as_repeat(this));
        case AST_RETURN:    return code_gen_return(func, as_return(this));
        case AST_IF:        return code_gen_if(func, as_if(this));
        case AST_CLAUSE:    fatal("gen_code clause should not be called directly (only from if)");
        case AST_MEMBER:    return code_gen_member(func, as_member(this));
        case AST_DECL:      return code_gen_decl(func, as_decl(this));
        case AST_FUNCTION:  return code_gen_function(as_function(this));
        case AST_BINOP:     return code_gen_binop(func, as_binop(this));
        case AST_UNARY:     return code_gen_unary(func, as_unary(this));
        case AST_FOR:       return code_gen_for(func, as_for(this));
        case AST_STRUCT:    TODO("");
    }
    fatal("Should not reach here");
}

// ---------------------------------------------------------------------------------
//                         code_gen_lvalue
// ---------------------------------------------------------------------------------
// Generate IR code for blocks on LHS of a scalar assignment

void code_gen_lvalue(Function func, AST this, Symbol value) {
    switch(this->kind) {
        case AST_SYMBOL:    return code_gen_lvalue_symbol(func, as_symbol(this), value);
        case AST_INDEX:     return code_gen_lvalue_index(func, as_index(this), value);
        case AST_POINTER:   return code_gen_lvalue_pointer(func, as_pointer(this), value);
        case AST_MEMBER:    return code_gen_lvalue_member(func, as_member(this), value);

        case AST_INTLIT:    
        case AST_STRLIT:    
        case AST_CAST:    
        case AST_NEW:    
        case AST_ASSIGN:    
        case AST_ASSIGNOP:    
        case AST_FUNCCALL:  
        case AST_WHILE:     
        case AST_REPEAT:    
        case AST_RETURN:    
        case AST_IF:        
        case AST_CLAUSE:    
        case AST_DECL:      
        case AST_FUNCTION:  
        case AST_BINOP:     
        case AST_UNARY:     
        case AST_FOR:     
        case AST_STRUCT:    fatal("Internal error - got %x in code_gen_lvalue", this->kind);
    }
}

// ---------------------------------------------------------------------------------
//                         code_gen_store_at
// ---------------------------------------------------------------------------------
// Generate IR code to store aggregate data at a given address

void code_gen_store_at(Function func, AST this, Symbol addr) {
    switch(this->kind) {
        case AST_SYMBOL:    return code_gen_store_at_symbol(func, as_symbol(this), addr);
        case AST_INDEX:     return code_gen_store_at_index(func, as_index(this), addr);
        case AST_POINTER:   TODO("");
        case AST_MEMBER:    TODO("");
        case AST_FUNCCALL:  return code_gen_store_at_funccall(func, as_funccall(this), addr);

        case AST_INTLIT:    
        case AST_STRLIT:    
        case AST_CAST:    
        case AST_NEW:    
        case AST_ASSIGN:    
        case AST_ASSIGNOP:    
        case AST_WHILE:     
        case AST_REPEAT:    
        case AST_RETURN:    
        case AST_IF:        
        case AST_CLAUSE:    
        case AST_DECL:      
        case AST_FUNCTION:  
        case AST_BINOP:     
        case AST_UNARY:     
        case AST_FOR:     
        case AST_STRUCT:    fatal("Internal error - got %x in code_gen_store_at", this->kind);
    }
}

// ---------------------------------------------------------------------------------
//                         code_gen_address_of
// ---------------------------------------------------------------------------------
// Generate IR code for the LHS of an aggregate operation

Symbol code_gen_address_of(Function func, AST this) {
    (void) func;
    switch(this->kind) {
        case AST_SYMBOL:    return code_gen_address_of_symbol(func, as_symbol(this));
        case AST_INDEX:     return code_gen_address_of_index(func, as_index(this));
        case AST_POINTER:   TODO("");
        case AST_MEMBER:    return code_gen_address_of_member(func, as_member(this));

        case AST_INTLIT:    
        case AST_STRLIT:    
        case AST_CAST:    
        case AST_NEW:    
        case AST_ASSIGN:    
        case AST_ASSIGNOP:    
        case AST_FUNCCALL:  
        case AST_WHILE:     
        case AST_REPEAT:    
        case AST_RETURN:    
        case AST_IF:        
        case AST_CLAUSE:    
        case AST_DECL:      
        case AST_FUNCTION:  
        case AST_BINOP:     
        case AST_UNARY:     
        case AST_FOR:     
        case AST_STRUCT:    fatal("Internal error - got %x in code_gen_address_of", this->kind);
    }
}

// ---------------------------------------------------------------------------------
//                         code_gen_bool
// ---------------------------------------------------------------------------------
// Generate IR code for a branch context

void code_gen_bool(Function func, AST this, Symbol lab_true, Symbol lab_false) {
    if (this->kind==AST_BINOP)
        return code_gen_bool_binop(func, as_binop(this), lab_true, lab_false);
    
    Symbol sym = code_gen(func, this);
    add_instr(func, new_Instr(INSTR_BRANCH, ALU_BNE_I, lab_true, sym, SymbolList_get(func->all_vars, 0)));
    add_instr(func, new_Instr(INSTR_JUMP,  0, lab_false,0, 0));
}
