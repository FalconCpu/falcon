#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fpl.h"
#include "token.h"

static Token lookahead;
static Block current_block;

static AST parse_expression();
static AST_list parse_expression_list();
static Block parse_block();
static AST parse_type_expr();

// ===========================================================================
//                         next_token
// ===========================================================================

static Token next_token() {
    Token ret = lookahead;
    lookahead = read_token();
    return ret;
}


// ===========================================================================
//                         expect
// ===========================================================================

static Token expect(TokenKind kind) {
    Token ret;
    if (lookahead->kind==TOK_EOL)
        ret = lookahead;
    else 
        ret = next_token();
    
    if (ret->kind!=kind)
        error(lookahead->location, "Got '%s' when expecting '%s'", ret->text, token_kind_names[kind]);
    return ret;
}

// ===========================================================================
//                         can_take
// ===========================================================================

static int can_take(TokenKind kind) {
    if (lookahead->kind==kind) {
        next_token();
        return 1;
    } else
        return 0;
}


// ===========================================================================
//                         skip_to_eol
// ===========================================================================

static void skip_to_eol() {
    while(lookahead->kind!=TOK_EOL && lookahead->kind!=TOK_EOF)
        next_token();
    while (lookahead->kind==TOK_EOL)
        next_token();
}

// ===========================================================================
//                         expect_eol
// ===========================================================================

static void expect_eol() {
    if (lookahead->kind!=TOK_EOL && lookahead->kind!=TOK_EOF)
        error(lookahead->location, "Got %s when expecting end of line", lookahead->text);
    skip_to_eol();
}

// ===========================================================================
//                         parse_intlit
// ===========================================================================

static AST parse_intlit() {
    Token tok = expect(TOK_INTLIT);

    char *endpointer;
    long value = strtoul(tok->text, &endpointer, 0);

    // TODO - add support for short/char
    if (endpointer != tok->text + strlen(tok->text))
        error(tok->location, "Maldormed int literal");

    return new_AST_intlit(tok->location, type_int, value);

}

// ===========================================================================
//                         parse_strlit
// ===========================================================================

static AST parse_strlit() {
    Token tok = expect(TOK_STRLIT);

    return new_AST_strlit(tok->location, type_string, tok->text);

}

// ===========================================================================
//                         parse_id
// ===========================================================================

static AST parse_id() {
    Token tok = expect(TOK_ID);
    return new_AST_symbol(tok->location, tok->text);
}

// ===========================================================================
//                         parse_bracket
// ===========================================================================

static AST parse_bracket() {
    expect(TOK_OPENB);
    AST ret = parse_expression();
    expect(TOK_CLOSEB);
    return ret;
}


// ===========================================================================
//                         parse_primary
// ===========================================================================

static AST parse_primary() {
    switch(lookahead->kind) {
        case TOK_INTLIT: return parse_intlit();
        case TOK_STRLIT: return parse_strlit();
        case TOK_ID:     return parse_id();
        case TOK_OPENB:  return parse_bracket();
        default:         error(lookahead->location,"Got '%s' when expecting primary expression", lookahead->text);
                         return 0;
    }
}

// ===========================================================================
//                         parse_index
// ===========================================================================

static AST parse_index(AST lhs) {
    Token tok = expect(TOK_OPENSQ);
    AST rhs = parse_expression();
    expect(TOK_CLOSESQ);
    return new_AST_index(tok->location, lhs, rhs);
}

// ===========================================================================
//                         parse_member
// ===========================================================================

static AST parse_member(AST lhs) {
    Token tok = next_token();
    Token id = expect(TOK_ID);
    return new_AST_member(tok->location, lhs, id->text, tok->kind==TOK_QMARKDOT);
}

// ===========================================================================
//                         parse_funccall
// ===========================================================================

static AST parse_funccall(AST lhs) {
    Token tok = expect(TOK_OPENB);
    AST_list rhs = parse_expression_list();
    expect(TOK_CLOSEB);
    return new_AST_funccall(tok->location, lhs, rhs);
}



// ===========================================================================
//                         parse_postfix
// ===========================================================================

static AST parse_postfix() {
    AST ret = parse_primary();
    while (1)
        if (lookahead->kind==TOK_OPENSQ)
            ret = parse_index(ret);
        else if (lookahead->kind==TOK_DOT || lookahead->kind==TOK_QMARKDOT)
            ret = parse_member(ret);
        else if (lookahead->kind==TOK_OPENB)
            ret = parse_funccall(ret);
        else
            return ret;
}

// ===========================================================================
//                         parse_prefix
// ===========================================================================

static AST parse_prefix() {
    if (lookahead->kind==TOK_MINUS || lookahead->kind==TOK_NOT) {
        Token t = next_token();
        return new_AST_unary(t->location, t->kind, parse_prefix());
    } else if (lookahead->kind==TOK_STAR) {
        Token t = next_token();
        return new_AST_pointer(t->location, parse_prefix());
    } else
        return parse_postfix();
}

// ===========================================================================
//                         parse_mult
// ===========================================================================

static AST parse_mult() {
    AST ret = parse_prefix();
    while(lookahead->kind==TOK_STAR || lookahead->kind==TOK_SLASH || lookahead->kind==TOK_PERCENT ||
          lookahead->kind==TOK_AMPERSAND || lookahead->kind==TOK_LEFT || lookahead->kind==TOK_RIGHT) {
        Token op = next_token();
        ret = new_AST_binop(op->location, op->kind, ret, parse_prefix());
    }
    return ret;
}

// ===========================================================================
//                         parse_add
// ===========================================================================

static AST parse_add() {
    AST ret = parse_mult();
    while(lookahead->kind==TOK_PLUS || lookahead->kind==TOK_MINUS || lookahead->kind==TOK_BAR ||
          lookahead->kind==TOK_CARAT) {
        Token op = next_token();
        ret = new_AST_binop(op->location, op->kind, ret, parse_mult());
    }
    return ret;
}

// ===========================================================================
//                         parse_comp
// ===========================================================================

static AST parse_comp() {
    AST ret = parse_add();
    while(lookahead->kind==TOK_EQ || lookahead->kind==TOK_NEQ || lookahead->kind==TOK_LT ||
          lookahead->kind==TOK_LTE || lookahead->kind==TOK_GT || lookahead->kind==TOK_GTE) {
        Token op = next_token();
        ret = new_AST_binop(op->location, op->kind, ret, parse_add());
    }
    return ret;
}

// ===========================================================================
//                         parse_and
// ===========================================================================

static AST parse_and() {
    AST ret = parse_comp();
    while(lookahead->kind==TOK_AND) {
        Token op = next_token();
        ret = new_AST_binop(op->location, op->kind, ret, parse_comp());
    }
    return ret;
}

// ===========================================================================
//                         parse_or
// ===========================================================================

static AST parse_or() {
    AST ret = parse_and();
    while(lookahead->kind==TOK_OR) {
        Token op = next_token();
        ret = new_AST_binop(op->location, op->kind, ret, parse_and());
    }
    return ret;
}

// ===========================================================================
//                         parse_expression
// ===========================================================================

static AST parse_expression() {
    AST ret = parse_or();
    return ret;
}

// ===========================================================================
//                         parse_expression_list
// ===========================================================================

static AST_list parse_expression_list() {
    AST_list ret = new_AST_list();
    if (lookahead->kind==TOK_CLOSEB)
        return ret;
    do {
        AST_list_add(ret, parse_expression());
    } while(can_take(TOK_COMMA));
    return ret;
}


// ===========================================================================
//                         parse_decl_statemet
// ===========================================================================

static AST parse_decl_statement() {
    Token dec = next_token();
    Token id = expect(TOK_ID);
    AST ast_type = 0;
    AST ast_value = 0;
    if (can_take(TOK_COLON))
        ast_type = parse_type_expr();
    if (can_take(TOK_EQ))
        ast_value = parse_expression();
    expect_eol();
    int mutable = (dec->kind==TOK_VAR);
    return new_AST_decl(id->location, id->text, ast_type, ast_value, mutable);
}

// ===========================================================================
//                         parse_return
// ===========================================================================

static AST parse_return_statement() {
    Token tok = expect(TOK_RETURN);
    AST retval = (lookahead->kind==TOK_EOL) ? 0 : parse_expression();
    expect_eol();
    return new_AST_return(tok->location, retval);
}

// ===========================================================================
//                         parse_assign
// ===========================================================================

static AST parse_assign_statement() {
    AST lhs = parse_prefix();
    if (lookahead->kind==TOK_EOL) {
        if (lhs->kind!=AST_FUNCCALL)
            warn(lhs->location, "Expression has no effect");
        return lhs;
    }

    Token loc = expect(TOK_EQ);
    AST rhs = parse_expression();
    expect_eol();

    return new_AST_assign(loc->location, lhs, rhs);
}

// ===========================================================================
//                         check_end
// ===========================================================================

static void check_end(TokenKind kind) {
    if (can_take(TOK_END)) {
        expect(kind);
        expect_eol();
    }
}

// ===========================================================================
//                         parse_while
// ===========================================================================

static AST parse_while_statement() {
    Token loc = expect(TOK_WHILE);
    AST condition = parse_expression();
    expect_eol();
    Block block = parse_block(loc->location, TOK_WHILE);
    check_end(TOK_WHILE);

    AST ret = new_AST_while(loc->location, condition, block);
    if (block)
        block->enclosing_statement = ret;
    return ret;
}


// ===========================================================================
//                         parse_repeat
// ===========================================================================

static AST parse_repeat_statement() {
    Token loc = expect(TOK_REPEAT);
    expect_eol();
    Block block = parse_block(loc->location, TOK_REPEAT);

    if (!can_take(TOK_UNTIL)) {
        error(lookahead->location,"Repeat loop misssing until");
        return 0;
    }
    AST condition = parse_expression();
    expect_eol();
    AST ret = new_AST_repeat(loc->location, condition, block);
    if (block)
        block->enclosing_statement = ret;
    return ret;
}

// ===========================================================================
//                         parse_param
// ===========================================================================

static AST parse_param() {
    Token id = expect(TOK_ID);
    expect(TOK_COLON);
    AST type = parse_type_expr();
    return new_AST_decl(id->location, id->text, type, 0, 0);
}

// ===========================================================================
//                         parse_param_list
// ===========================================================================

static AST_list parse_param_list() {
    if (!can_take(TOK_OPENB)) {
        error(lookahead->location, "Function has no parameter list");
        return 0;
    }

    AST_list ret = new_AST_list();
    if (can_take(TOK_CLOSEB))
        return ret;

    do {
        AST_list_add(ret, parse_param());
    } while (can_take(TOK_COMMA));

    expect(TOK_CLOSEB);
    return ret;
}

// ===========================================================================
//                         parse_clause
// ===========================================================================

static AST parse_clause() {
    Token loc = next_token();
    AST condition = parse_expression();
    expect_eol();
    Block block = parse_block(loc->location, TOK_IF);
    AST ret =  new_AST_clause(loc->location, condition, block);
    block->enclosing_statement = ret;
    return ret;
}

static AST parse_else_clause() {
    Token loc = next_token();
    expect_eol();
    Block block = parse_block(loc->location, TOK_ELSE);

    AST ret = new_AST_clause(loc->location, 0, block);
    block->enclosing_statement = ret;
    return ret;
}


// ===========================================================================
//                         parse_if
// ===========================================================================

static AST parse_if_statement() {
    Location location = lookahead->location;

    Block block = new_block(current_block);
    current_block = block;
    block_add_statement(block, parse_clause());
    while(lookahead->kind==TOK_ELSIF) 
        block_add_statement(block, parse_clause());
    if (lookahead->kind==TOK_ELSE)
        block_add_statement(block, parse_else_clause());
    check_end(TOK_IF);
    current_block = current_block->parent;

    return new_AST_if(location, block);
}

// ===========================================================================
//                         parse_function
// ===========================================================================

static AST parse_function_statement() {
    Token loc = expect(TOK_FUN);
    Token name = expect(TOK_ID);
    AST_list params = parse_param_list();
    AST ret_type = can_take(TOK_ARROW) ? parse_type_expr() : 0;
    expect_eol();
    Block body = parse_block(loc->location, TOK_FUN);
    check_end(TOK_FUN);

    AST ret = new_function(loc->location, name->text, params, ret_type, body);
    body->enclosing_statement = ret;
    return ret;
}

// ===========================================================================
//                         parse_struct
// ===========================================================================

static AST parse_struct_statement() {
    Token loc = expect(TOK_STRUCT);
    Token name = expect(TOK_ID);
    AST_list params = parse_param_list();
    expect_eol();

    AST ret = new_AST_struct(loc->location, name->text, params);
    return ret;
}



// ===========================================================================
//                         parse_type_primary
// ===========================================================================

static AST parse_type_primary() {
    // TODO - add support for brackets
    Token tok = expect(TOK_ID);
    return new_AST_symbol(tok->location, tok->text);
}


// ===========================================================================
//                         parse_type_expr
// ===========================================================================

static AST parse_type_expr() {
    AST ret = parse_type_primary();
    Token loc;
    while(1) {
        switch(lookahead->kind) {
            case TOK_STAR:
                next_token();
                ret = new_AST_pointer(ret->location, ret);
                break;

            case TOK_OPENSQ:
                loc = next_token();
                AST arg = lookahead->kind==TOK_CLOSESQ ? 0 : parse_expression();
                expect(TOK_CLOSESQ);
                ret = new_AST_index(loc->location, ret, arg);
                break;

            default:
                return ret;
        }
    }
}

// ===========================================================================
//                         parse_statement
// ===========================================================================

static AST parse_statement() {

    switch(lookahead->kind) {
        case TOK_VAL:
        case TOK_VAR:       return parse_decl_statement();
        case TOK_RETURN:    return parse_return_statement();
        case TOK_ID:     
        case TOK_OPENB:     return parse_assign_statement();
        case TOK_WHILE:     return parse_while_statement();
        case TOK_REPEAT:    return parse_repeat_statement();
        case TOK_IF:        return parse_if_statement();
        case TOK_FUN:       return parse_function_statement();
        case TOK_STRUCT:    return parse_struct_statement();
        default:            error(lookahead->location,"Got '%s' when expecting statement", lookahead->text);
                            skip_to_eol();
                            return 0;
    }
}

// ===========================================================================
//                         parse_block
// ===========================================================================

static Block parse_block(Location location, TokenKind kind) {
   if (!can_take(TOK_INDENT)) {
        error(location,"%s statement has no body", token_kind_names[kind]);
        return 0;
    }
    
    Block block = new_block(current_block);
    current_block = block;
    while(lookahead->kind!=TOK_EOF && lookahead->kind!=TOK_DEDENT)
        block_add_statement(block, parse_statement());
    current_block = current_block->parent;
    expect(TOK_DEDENT);
    return block;
}


// ===========================================================================
//                         parse_top
// ===========================================================================

void parse_top(Block block) {
    current_block = block;
    next_token();
    while(lookahead->kind!=TOK_EOF)
        block_add_statement(block, parse_statement());
}


