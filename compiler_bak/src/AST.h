#include "fpl.h"
#include "token.h"
#include "instr.h"

// -----------------------------------------------------------------------
//                        AST_INTLIT
// -----------------------------------------------------------------------

typedef struct AST_intlit* AST_intlit;

struct AST_intlit {
    Location location;
    ASTkind  kind;
    Type     type;

    int      value;
};

void AST_intlit_print(AST_intlit this, int indent);

AST_intlit as_intlit(AST this);

void AST_typecheck_intlit(AST_intlit this, Block scope);

Symbol code_gen_intlit(Function func, AST_intlit this);

// -----------------------------------------------------------------------
//                        AST_STRLIT
// -----------------------------------------------------------------------

typedef struct AST_strlit* AST_strlit;

struct AST_strlit {
    Location location;
    ASTkind  kind;
    Type     type;

    String   value;
};

void AST_strlit_print(AST_strlit this, int indent);

AST_strlit as_strlit(AST this);

void AST_typecheck_strlit(AST_strlit this, Block scope);

Symbol code_gen_strlit(Function func, AST_strlit this);

// -----------------------------------------------------------------------
//                        AST_SYMBOL
// -----------------------------------------------------------------------

typedef struct AST_symbol* AST_symbol;

struct AST_symbol {
    Location location;
    ASTkind  kind;
    Type     type;

    String   name;
    Symbol   symbol;   // filled in at typechecking
    Value    value;    // symbol's value - if known
};

void AST_symbol_print(AST_symbol this, int indent);

AST_symbol as_symbol(AST this);

void AST_typecheck_symbol(AST_symbol this, Block scope);

Symbol code_gen_symbol(Function func, AST_symbol this);

void code_gen_lvalue_symbol(Function func, AST_symbol this, Symbol value);

Symbol  code_gen_address_of_symbol(Function func, AST_symbol this);

void  code_gen_store_at_symbol(Function func, AST_symbol this, Symbol dest);

// -----------------------------------------------------------------------
//                        AST_BINOP
// -----------------------------------------------------------------------

typedef struct AST_binop* AST_binop;

struct AST_binop {
    Location  location;
    ASTkind   kind;
    Type      type;

    TokenKind op;
    AST       lhs;
    AST       rhs;
    AluOp     alu_op;
};

void AST_binop_print(AST_binop this, int indent);

AST_binop as_binop(AST this);

void AST_typecheck_binop(AST_binop this, Block scope);

void  code_gen_bool_binop(Function func, AST_binop this, Symbol label_true, Symbol label_false);

Symbol code_gen_binop(Function func, AST_binop this);


// -----------------------------------------------------------------------
//                        AST_NEW
// -----------------------------------------------------------------------

typedef struct AST_new* AST_new;

struct AST_new {
    Location  location;
    ASTkind   kind;
    Type      type;

    AST       rhs;
};

void AST_new_print(AST_new this, int indent);

AST_new as_new(AST this);

void AST_typecheck_new(AST_new this, Block scope);

Symbol code_gen_new(Function func, AST_new this);


// -----------------------------------------------------------------------
//                        AST_CAST
// -----------------------------------------------------------------------

typedef struct AST_cast* AST_cast;

struct AST_cast {
    Location  location;
    ASTkind   kind;
    Type      type;
    AST       expr;
    AST       type_expr;
};

void AST_cast_print(AST_cast this, int indent);

AST_cast as_cast(AST this);

void AST_typecheck_cast(AST_cast this, Block scope);

Symbol code_gen_cast(Function func, AST_cast this);


// -----------------------------------------------------------------------
//                        AST_POINTER
// -----------------------------------------------------------------------

typedef struct AST_pointer* AST_pointer;

struct AST_pointer {
    Location  location;
    ASTkind   kind;
    Type      type;

    AST       rhs;
};

void AST_pointer_print(AST_pointer this, int indent);

AST_pointer as_pointer(AST this);

void AST_typecheck_pointer(AST_pointer this, Block scope);

Symbol code_gen_pointer(Function func, AST_pointer this);

void code_gen_lvalue_pointer(Function func, AST_pointer this, Symbol value);

// -----------------------------------------------------------------------
//                        AST_UNARY
// -----------------------------------------------------------------------

typedef struct AST_unary* AST_unary;

struct AST_unary {
    Location  location;
    ASTkind   kind;
    Type      type;

    TokenKind op;
    AST       rhs;
};

void AST_unary_print(AST_unary this, int indent);

AST_unary as_unary(AST this);

void AST_typecheck_unary(AST_unary this, Block scope);

Symbol code_gen_unary(Function func, AST_unary this);

// -----------------------------------------------------------------------
//                        AST_INDEX
// -----------------------------------------------------------------------

typedef struct AST_index* AST_index;

struct AST_index {
    Location  location;
    ASTkind   kind;
    Type      type;

    AST       lhs;
    AST       rhs;

    int       is_constructor; // filled in at type checking
};

void AST_index_print(AST_index this, int indent);

AST_index as_index(AST this);

void AST_typecheck_index(AST_index this, Block scope);

Symbol code_gen_index(Function func, AST_index this);

void code_gen_lvalue_index(Function func, AST_index this, Symbol value);

Symbol code_gen_address_of_index(Function func, AST_index this);

void  code_gen_store_at_index(Function func, AST_index this, Symbol dest);

// -----------------------------------------------------------------------
//                        AST_MEMBER
// -----------------------------------------------------------------------

typedef struct AST_member* AST_member;

struct AST_member {
    Location  location;
    ASTkind   kind;
    Type      type;

    AST       lhs;
    String    name;
    int       null_coalese;

    Symbol    symbol;   // filled in at type checking
};

void AST_member_print(AST_member this, int indent);

AST_member as_member(AST this);

void AST_typecheck_member(AST_member this, Block scope);

Symbol code_gen_member(Function func, AST_member this);

void code_gen_lvalue_member(Function func, AST_member this, Symbol value);

Symbol code_gen_address_of_member(Function func, AST_member this);

// -----------------------------------------------------------------------
//                        AST_FUNCCALL
// -----------------------------------------------------------------------

typedef struct AST_funccall* AST_funccall;

struct AST_funccall {
    Location  location;
    ASTkind   kind;
    Type      type;

    AST       lhs;
    AST_list  rhs;

    int       is_constructor;
};

void AST_funccall_print(AST_funccall this, int indent);

AST_funccall as_funccall(AST this);

void AST_typecheck_funccall(AST_funccall this, Block scope);

Symbol code_gen_funccall(Function func, AST_funccall this);

void code_gen_store_at_funccall(Function func, AST_funccall this, Symbol dest);
// -----------------------------------------------------------------------
//                        AST_DECL
// -----------------------------------------------------------------------

typedef struct AST_decl* AST_decl;

struct AST_decl {
    Location  location;
    ASTkind   kind;
    Type      type;

    String    name;
    AST       ast_type;
    AST       ast_value;
    int       mutable;

    Symbol    symbol;   // filled in at type checking
};

void AST_decl_print(AST_decl this, int indent);

AST_decl as_decl(AST this);

void AST_typecheck_decl(AST_decl this, Block scope);

Symbol code_gen_decl(Function func, AST_decl this);


// -----------------------------------------------------------------------
//                        AST_RETURN
// -----------------------------------------------------------------------

typedef struct AST_return* AST_return;

struct AST_return {
    Location  location;
    ASTkind   kind;
    Type      type;

    AST       retval;
};

void AST_return_print(AST_return this, int indent);

AST_return as_return(AST this);

void AST_typecheck_return(AST_return this, Block scope);

Symbol code_gen_return(Function func, AST_return this);

// -----------------------------------------------------------------------
//                        AST_ASSIGN
// -----------------------------------------------------------------------

typedef struct AST_assign* AST_assign;

struct AST_assign {
    Location  location;
    ASTkind   kind;
    Type      type;

    AST       lhs;
    AST       rhs;
};

void AST_assign_print(AST_assign this, int indent);

AST_assign as_assign(AST this);

void AST_typecheck_assign(AST_assign this, Block scope);

Symbol code_gen_assign(Function func, AST_assign this);

// -----------------------------------------------------------------------
//                        AST_ASSIGNOp
// -----------------------------------------------------------------------

typedef struct AST_assignOp* AST_assignOp;

struct AST_assignOp {
    Location  location;
    ASTkind   kind;
    Type      type;

    TokenKind op;
    AST       lhs;
    AST       rhs;
};

void AST_assignOp_print(AST_assignOp this, int indent);

AST_assignOp as_assignOp(AST this);

void AST_typecheck_assignOp(AST_assignOp this, Block scope);

Symbol code_gen_assignOp(Function func, AST_assignOp this);


// -----------------------------------------------------------------------
//                        AST_WHILE
// -----------------------------------------------------------------------

typedef struct AST_while* AST_while;

struct AST_while {
    Location  location;
    ASTkind   kind;
    Type      type;

    AST       condition;
    Block     body;
};

void AST_while_print(AST_while this, int indent);

AST_while as_while(AST this);

void AST_typecheck_while(AST_while this, Block scope);

Symbol code_gen_while(Function func, AST_while this);

// -----------------------------------------------------------------------
//                        AST_REPEAT
// -----------------------------------------------------------------------

typedef struct AST_repeat* AST_repeat;

struct AST_repeat {
    Location  location;
    ASTkind   kind;
    Type      type;

    AST       condition;
    Block     body;
};

void AST_repeat_print(AST_repeat this, int indent);

AST_repeat as_repeat(AST this);

void AST_typecheck_repeat(AST_repeat this, Block scope);

Symbol code_gen_repeat(Function func, AST_repeat this);

// -----------------------------------------------------------------------
//                        AST_CLAUSE
// -----------------------------------------------------------------------

typedef struct AST_clause* AST_clause;

struct AST_clause {
    Location  location;
    ASTkind   kind;
    Type      type;

    AST       condition;
    Block     body;
};

void AST_clause_print(AST_clause this, int indent);

AST_clause as_clause(AST this);

void AST_typecheck_clause(AST_clause this, Block scope);

Symbol code_gen_clause(Function func, AST_clause this, Symbol lab_next, Symbol lab_end);


// -----------------------------------------------------------------------
//                        AST_IF
// -----------------------------------------------------------------------

typedef struct AST_if* AST_if;

struct AST_if {
    Location  location;
    ASTkind   kind;
    Type      type;

    Block     clauses;
};

void AST_if_print(AST_if this, int indent);

AST_if as_if(AST this);

void AST_typecheck_if(AST_if this, Block scope);

Symbol code_gen_if(Function func, AST_if this);

// -----------------------------------------------------------------------
//                        AST_STRUCT
// -----------------------------------------------------------------------

typedef struct AST_struct* AST_struct;

struct AST_struct {
    Location  location;
    ASTkind   kind;
    Type      type;

    String    name;
    AST_list  members;
    Block     body;
};

void AST_struct_print(AST_struct this, int indent);

AST_struct as_struct(AST this);

void AST_typecheck_struct(AST_struct this, Block scope);

// -----------------------------------------------------------------------
//                        AST_FOR
// -----------------------------------------------------------------------

typedef struct AST_for* AST_for;

struct AST_for{
    Location location;
    ASTkind  kind;       // AST_FOR
    Type     type;
    String   id_name;
    AST      start_expr;
    AST      end_expr;
    Block    body;

    // filled in at typecheck
    Symbol   iterator;  // Symbol for iterator
};

void AST_for_print(AST_for this, int indent);

AST_for as_for(AST this);

void AST_typecheck_for(AST_for this, Block scope);

Symbol code_gen_for(Function func, AST_for this);

// -----------------------------------------------------------------------
//                        FUNCTION
// -----------------------------------------------------------------------

// main definition in fpl.h

void AST_function_print(Function this, int indent);

void AST_typecheck_function(Function this, Block scope);



// -----------------------------------------------------------------------
//                        AST
// -----------------------------------------------------------------------

void print_spaces(int indent);

Type resolve_AST_type(AST this, Block scope);

void block_typecheck(Block block);
