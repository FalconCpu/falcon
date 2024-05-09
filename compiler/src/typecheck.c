#include "fpl.h"
#include "AST.h"
#include "types.h"

Type resolve_AST_type(AST this, Block scope);

// ---------------------------------------------------------------------------------
//                         AST_identify_structs
// ---------------------------------------------------------------------------------
// Walk through an AST, identifying any structs and adding them as symbols
// to the block which contains them

void AST_identify_structs(Block this) {
    if (this==0)
        return;

    AST stmt;
    foreach_statement(stmt, this) {
        if (stmt->kind==AST_STRUCT) {
            AST_struct ast = as_struct(stmt);
            ast->type = make_type_struct(ast->name, ast->body);
            Symbol sym = new_Symbol(ast->location, SYM_TYPEREF, ast->name, ast->type, 0);
            as_TypeStruct(ast->type)->symbol = sym;
            block_add_symbol(this, sym);
        }

        // recurse into any sub-blocks of this statement
        AST_identify_structs(AST_get_block(stmt));
    }
}


extern SymbolList builtin_types;

// ---------------------------------------------------------------------------------
//                         resolve_AST_typelist
// ---------------------------------------------------------------------------------
// Helper function for resolve_AST_type
// Converts a type expression into a TypeList. The input could be a single type, or it
// could be a list of types wrapped up as the arguments of a func call

static TypeList resolve_AST_typelist(AST this, Block scope) {
    TypeList ret = new_TypeList();

    if (this->kind == AST_FUNCCALL) {
        AST_funccall this_funccall = as_funccall(this);
        AST n;
        foreach(n, this_funccall->rhs)
            TypeList_add(ret, resolve_AST_type(n, scope));

    } else {
        // Just got a single type in the list
        TypeList_add(ret, resolve_AST_type(this,scope));
    }

    return ret;
}

// ---------------------------------------------------------------------------------
//                         resolve_AST_type
// ---------------------------------------------------------------------------------
// Types are initially desribed in the AST 'as-if' they were expressions.
//
// *  named types (primatives and structs) are represented as Identifiers
// *  Pointer types are represented as pointer dereference operators
// *  Nullable Pointer types are represented as nullchk operators
// *  Arrays are represented as array indexing operations (with the array size, if known, being the index value)
// *  Function types are represented as a binary -> operators (with the functions parameters being the LHS, and return type the RHS)
// *  Where types are grouped (eg. (Int,Int)) this represented as a function call (with the types being the arguments)
//
// Given an AST which describes a such a type resolve into a reference to a Type instance

Type resolve_AST_type(AST this, Block scope) {

    switch(this->kind) {
        case AST_SYMBOL: {
            // An identifier will represent a named type, either a builtin primative or a user defined type
            AST_symbol ast_symbol = as_symbol(this);
            Symbol sym = SymbolList_find(builtin_types, ast_symbol->name);
            if (sym==0)
                sym = block_find_symbol(scope, ast_symbol->name);
            if (sym==0)
                return make_type_error(ast_symbol->location, "Undefined symbol '%s'", ast_symbol->name);
            if (sym->kind != SYM_TYPEREF)
                return make_type_error(ast_symbol->location, "Symbol '%s' descries a %s not a type", ast_symbol->name, symbol_kind_name[sym->kind]);
            return sym->type;
        }

        case AST_INDEX: {
            AST_index this_index = as_index(this);
            Type base_type = resolve_AST_type(this_index->lhs, scope);
            if (base_type==type_error)
                return type_error;
            // Arrays of unknown size are represented as size zero
            int array_size = 0;
            if (this_index->rhs != 0) {
                AST_typecheck(this_index->rhs, scope);
                check_type_compatible(type_int, this_index->rhs);
                array_size = AST_get_constant_value(this_index->rhs);
            }
            return make_type_array(base_type, array_size);
        }

        // case AST_NULLCHK: {
        //     AST_nullchk this_nullchk = asNullchk(this);
        //     Type base_type = resolve_AST_type(this_nullchk->lhs, scope);
        //     if (base_type==type_error)
        //         return type_error;
        //     return make_type_pointer(base_type, 1);
        // }

        case AST_POINTER: {
            AST_pointer this_pointer = as_pointer(this);
            Type base_type = resolve_AST_type(this_pointer->rhs, scope);
            if (base_type==type_error)
                return type_error;
            return make_type_pointer(base_type, 0);
        }

        case AST_BINOP: {
            AST_binop this_binop = as_binop(this);
            if (this_binop->op != TOK_ARROW)
                fatal("Internal error - got kind %03x as binop operator in resolve_AST_type");
            TypeList params = resolve_AST_typelist(this_binop->lhs, scope);
            Type return_type = resolve_AST_type(this_binop->rhs, scope);
            return make_type_function(params, return_type);
        }

        case AST_FUNCCALL:
            error(this->location, "Multiple types not allowed in this context");

        default:
            fatal("Internal error: Got kind %03x in resolve_AST_type", this->kind);
    }
}

// ---------------------------------------------------------------------------------
//                         resolve_AST_decl
// ---------------------------------------------------------------------------------
// resolve an AST_decl node into a symbol - with name and type
Symbol resolve_AST_decl(AST this, Block scope) {
    AST_decl this_decl = as_decl(this);
    return new_Symbol(this_decl->location, SYM_VARIABLE, this_decl->name, resolve_AST_type(this_decl->ast_type, scope), 0);
}

// ---------------------------------------------------------------------------------
//                         resolve_function_type
// ---------------------------------------------------------------------------------
// Given an AST which describes a function definition,
// populate its parameter and return_type and type fields.
//
// The function could be extern - in which case this->block ==0

Type resolve_function_type(Function this, Block scope) {
    this->parameters = new_SymbolList();
    TypeList param_types = new_TypeList();

    AST ast_param;
    foreach(ast_param, this->params) {
        Symbol sym_param = resolve_AST_decl(ast_param, scope);
        Symbol duplicate = SymbolList_find(this->parameters, sym_param->name);
        if (duplicate)
            error(ast_param->location, "Duplicate parameter '%s'", sym_param->name);
        TypeList_add(param_types, sym_param->type);
        SymbolList_add(this->parameters, sym_param);
        if (this->body)
            block_add_symbol(this->body, sym_param);
    }

    this->return_type = this->ret_type_ast ? resolve_AST_type(this->ret_type_ast, scope) : type_void;
    this->type = make_type_function(param_types, this->return_type);
    return this->type;
}

// ---------------------------------------------------------------------------------
//                         AST_identify_funcs_and_fields
// ---------------------------------------------------------------------------------
// Walk through the AST, identifying any function definitions and adding them as
// symbols to the appropriate symbol tables, and identifying the fields of structs

static void AST_identify_funcs_and_fields(Block block) {
    if (block == 0)
        return;

    AST stmt;
    foreach_statement(stmt, block) {
        switch(stmt->kind) {
            case AST_FUNCTION: {
                // We have found a function definition. Populate its fields and add a reference to our symbol table
                Function this_func = as_function(stmt);
                resolve_function_type(this_func, block);
                this_func->sym = new_Symbol(this_func->location, SYM_FUNCTION, this_func->name, this_func->type, 0);
                this_func->sym->value.function = this_func;
                block_add_symbol(block, this_func->sym);

                // if its a member function then add the 'this' symbol
                if (block->enclosing_statement && block->enclosing_statement->kind==AST_STRUCT) {
                    this_func->this_sym = new_Symbol(this_func->location, SYM_VARIABLE, unique_string("this"), make_type_pointer(block->enclosing_statement->type,0), 0);
                    block_add_symbol(this_func->body, this_func->this_sym);
                }
                break;
            }

            case AST_STRUCT: {
                // we have reached a struct definition. Populate its fields.
                AST_struct this_struct = as_struct(stmt);
                Type_struct type = as_TypeStruct(this_struct->type);
                AST ast_field;
                foreach(ast_field, this_struct->members) {
                    Symbol sym = resolve_AST_decl(ast_field, block);
                    sym->kind = SYM_FIELD;
                    block_add_symbol(this_struct->body, sym);
                    SymbolList_add(type->params, sym);
                }
                break;
            }

            default: break;
        }

        AST_identify_funcs_and_fields(AST_get_block(stmt));
    }
}

// ---------------------------------------------------------------------------------
//                         AST_typecheck_typeexpr
// ---------------------------------------------------------------------------------
// Runs typechecking, and verifies the result is a type expression

void block_typecheck(Block block) {
    AST stmt;
    foreach_statement(stmt, block)
        AST_typecheck(stmt, block);
}




// ---------------------------------------------------------------------------------
//                         is_type_expr
// ---------------------------------------------------------------------------------
// returns true if an expression represents a type

int is_type_expr(AST this) {
    // TODO - Symbol typeref's are currently the only way a type can appear in a regular
    // AST node, but this function will need updating later if other ways become possible
    return   (this->kind==AST_SYMBOL && as_symbol(this)->symbol->kind==SYM_TYPEREF) ||
             (this->kind==AST_INDEX && as_index(this)->is_constructor) ;
}

// ---------------------------------------------------------------------------------
//                         AST_typecheck_value
// ---------------------------------------------------------------------------------
// Runs typechecking, and verifies the result is a value

void AST_typecheck_value(AST this, Block scope) {
    AST_typecheck(this, scope);
    if (is_type_expr(this))
        error(this->location, "Expected a value expression, got type '%s'", this->type->name);
}

// ---------------------------------------------------------------------------------
//                         AST_typecheck_typeexpr
// ---------------------------------------------------------------------------------
// Runs typechecking, and verifies the result is a type expression

void AST_typecheck_typeexpr(AST this, Block scope) {
    AST_typecheck(this, scope);
    if (!is_type_expr(this))
        error(this->location, "Got a value expression when expecting a type");
}

// ---------------------------------------------------------------------------------
//                         typecheck
// ---------------------------------------------------------------------------------
// run semantic checking on the AST
// Because the language can have forward references to structs and functions we do
// the checking in three phases
//
// First find all the struct definitions, and add them into the symbol tables
// Second find all the function definitions, and add them
// Finally Check everything else
void typecheck(Block top) {
    AST_identify_structs(top);
    AST_identify_funcs_and_fields(top);
    block_typecheck(top);
}