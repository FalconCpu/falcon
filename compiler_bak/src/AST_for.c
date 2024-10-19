#include <stdio.h>
#include "fpl.h"
#include "instr.h"
#include "AST.h"

// ---------------------------------------------------------------------------------
//                         constructor
// ---------------------------------------------------------------------------------


AST new_AST_for(Location loc, String name, AST start_expr, AST end_expr, Block body) {
    AST_for ret = new(struct AST_for);
    ret->location = loc;
    ret->kind = AST_FOR;
    ret->start_expr = start_expr;
    ret->end_expr = end_expr;
    ret->id_name = name;
    ret->body = body;
    return as_AST(ret);
}

// ---------------------------------------------------------------------------------
//                         print
// ---------------------------------------------------------------------------------

void AST_for_print(AST_for node, int indent) {
    print_spaces(indent);
    printf("FOR %s\n", node->iterator ? Symbol_toText(node->iterator) : node->id_name);
    AST_print(node->start_expr, indent + 1);
    AST_print(node->end_expr, indent + 1);
    block_print(node->body, indent+1);
}

// ---------------------------------------------------------------------------------
//                         asWhile
// ---------------------------------------------------------------------------------

AST_for as_for(AST this) {
    if (this==0)
        fatal("Null pointer cast to ASTnode_for");
    if (this->kind!=AST_FOR)
        fatal("Attempt to cast %x to For", this->kind);
    return (AST_for)this;
}

// ---------------------------------------------------------------------------------
//                         typecheck
// ---------------------------------------------------------------------------------

void AST_typecheck_for(AST_for this, Block scope) {
    AST_typecheck_value(this->start_expr, scope);
    AST_typecheck_value(this->end_expr, scope);

    check_type_compatible(type_int, this->start_expr);  // TODO allow other iterable loop vars
    check_type_compatible(this->start_expr->type, this->end_expr);

    this->iterator = new_Symbol(this->location, SYM_VARIABLE, this->id_name, this->start_expr->type, 0);
    block_add_symbol(this->body, this->iterator);

    AST stmt;
    foreach_statement(stmt, this->body)
        AST_typecheck(stmt, this->body);
}

// ---------------------------------------------------------------------------------
//                           code_gen
// ---------------------------------------------------------------------------------

Symbol code_gen_for(Function func, AST_for this) {

    Symbol start_val = code_gen(func, this->start_expr);
    Symbol end_val = code_gen(func, this->end_expr);

    Symbol iter = get_codegen_symbol(func, this->iterator);
    add_instr(func, new_Instr(INSTR_MOV, 0, iter, start_val, 0));

    Symbol lab_start = new_label(func);
    Symbol lab_cond = new_label(func);
    Symbol lab_end = new_label(func);

    add_instr(func, new_Instr(INSTR_JUMP, 0, lab_cond, 0,0));
    add_instr(func, new_Instr(INSTR_LABEL , 0, lab_start, 0,0));
    code_gen_block(func, this->body);
    add_instr(func, new_Instr(INSTR_ALU, ALU_ADD_I, iter, iter, make_constant_symbol(func,1))); // TODO allow for other step sizes
    add_instr(func, new_Instr(INSTR_LABEL , 0, lab_cond, 0,0));
    add_instr(func, new_Instr(INSTR_BRANCH, ALU_BLE_I, lab_start, iter, end_val));
    add_instr(func, new_Instr(INSTR_LABEL , 0, lab_end, 0,0));
    return 0;
}