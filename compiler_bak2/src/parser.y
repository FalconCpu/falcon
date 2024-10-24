
%{
    #include "..\fpl.h"
    #include <stdio.h>

    #define YYLTYPE Location 

    extern Location yylloc;

    Block top_block;


    void yyerror(const char *s);
    int yylex(void);

    # define YYLLOC_DEFAULT(Current, Rhs, N) \
    do \
        if (N) \
        { \
        (Current).filename = YYRHSLOC(Rhs, 1).filename; \
        (Current).line = YYRHSLOC(Rhs, 1).line; \
        (Current).column = YYRHSLOC(Rhs, 1).column; \
        } \
        else \
        { \
        (Current).filename = YYRHSLOC(Rhs, 0).filename; \
        (Current).line = YYRHSLOC(Rhs, 0).line; \
        (Current).column = YYRHSLOC(Rhs, 0).column; \
        } \
    while (0)

%}

%token EOL        0x01 "end of line"
%token INDENT     0x02 "indent"
%token OUTDENT    0x03 "dedent"
%token IDENTIFIER 0x04 "identifier"
%token INTLIT     0x05 "int literal"
%token REALLIT    0x06 "real literal"
%token STRINGLIT  0x07 "string literal"
%token CHARLIT    0x08 "char literal"
%token PLUS       0x09 "+"
%token MINUS      0x0A "-"
%token STAR       0x0B "*"
%token SLASH      0x0C "/"
%token PERCENT    0x0D "%"
%token AMP        0x0E "&"
%token BAR        0x0F "|"
%token CARAT      0x10 "^"
%token EQ         0x11 "="
%token NEQ        0x12 "!="
%token LT         0x13 "<"
%token LTE        0x14 "<="
%token GT         0x15 ">"
%token GTE        0x16 ">="
%token NOT        0x17 "not"
%token AND        0x18 "and"
%token OR         0x19 "or"
%token DOT        0x1A "."
%token ARROW      0x1B "->"
%token COMMA      0x1C ","
%token COLON      0x1D ":"
%token OPENB      0x1E "("
%token OPENSQ     0x1F "["
%token OPENCL     0x20 "{"
%token CLOSEB     0x21 ")"
%token CLOSESQ    0x22 "]"
%token CLOSECL    0x23 "}"
%token VAL        0x24 "val"
%token VAR        0x25 "var"
%token IF         0x26 "if"
%token ELSE       0x27 "else"
%token END        0x28 "end"
%token WHILE      0x29 "while"
%token REPEAT     0x2A "repeat"
%token UNTIL      0x2B "until"
%token CLASS      0x2C "class"
%token ENUM       0x2D "enum"
%token RETURN     0x2E "return"
%token FUN        0x2F "fun"

%union {
    String string;
    AST    ast;
    Block  block;
    AST_list list;
}

%type <ast> expr primary postfix prefix mult add comp and or
%type <ast> top_level_stmt declaration expr_stmt opt_eq
%type <ast> type typename opt_type param  opt_func_ret_type
%type <ast> func_decl block_stmt
%type <block> block
%type <list> param_list1 param_list
%type <string> INTLIT REALLIT STRINGLIT CHARLIT IDENTIFIER

%error-verbose

%%

program:
    /* empty */ 
|   program top_level_stmt           { block_add_statement(top_block, $2); }

primary:
    INTLIT                  { $$ = new_ast_IntLiteral(@1, stringToInt($1)); }
|   IDENTIFIER              { $$ = new_ast_Identifier(@1, $1); }

postfix:
    primary

prefix: 
    postfix

mult:
    prefix
|    mult STAR prefix        { $$ = new_ast_Binop(@1, STAR, $1, $3) }
|    mult SLASH prefix       { $$ = new_ast_Binop(@1, SLASH, $1, $3) }
|    mult PERCENT prefix     { $$ = new_ast_Binop(@1, PERCENT, $1, $3) }
|    mult AMP prefix        { $$ = new_ast_Binop(@1, AMP, $1, $3) }

add:
    mult
|   add PLUS mult         { $$ = new_ast_Binop(@1, PLUS, $1, $3) }
|   add MINUS mult        { $$ = new_ast_Binop(@1, MINUS, $1, $3) }
|   add BAR mult          { $$ = new_ast_Binop(@1, BAR, $1, $3) }
|   add CARAT mult        { $$ = new_ast_Binop(@1, CARAT, $1, $3) }

comp:
    add
|   add EQ add            { $$ = new_ast_Binop(@1, EQ, $1, $3) }
|   add NEQ add           { $$ = new_ast_Binop(@1, NEQ, $1, $3) }
|   add LT add            { $$ = new_ast_Binop(@1, LT, $1, $3) }
|   add LTE add           { $$ = new_ast_Binop(@1, LTE, $1, $3) }
|   add GT add            { $$ = new_ast_Binop(@1, GT, $1, $3) }
|   add GTE add           { $$ = new_ast_Binop(@1, GTE, $1, $3) }

and:
    comp
|   and AND comp        { $$ = new_ast_Binop(@1, AND, $1, $3) }

or:
    and
|   or OR and           { $$ = new_ast_Binop(@1, OR, $1, $3) }


expr:
    or

opt_eq:
    /* empty */         { $$ = 0; }
|   EQ expr             { $$ = $2; }

typename:
    IDENTIFIER          { $$ = new_ast_TypeName(@1, $1); }

type:
    typename    

opt_type:
    /* empty */         { $$ = 0; }
|   COLON type          { $$ = $2; }


declaration:
    VAL IDENTIFIER opt_type opt_eq EOL { $$ = new_ast_Declaration(@1, VAL, $2, $3, $4); }
|   VAR IDENTIFIER opt_type opt_eq EOL { $$ = new_ast_Declaration(@1, VAR, $2, $3, $4); }

expr_stmt:
    expr EOL            { $$ = $1; }

top_level_stmt:
    expr_stmt
|   declaration
|   func_decl

param:
    IDENTIFIER COLON type    {$$ = new_ast_Parameter(@1, 0,  $1, $3); }

param_list1: 
    param                    { $$ = new_list(); list_add($$, $1); }
|   param_list1 COMMA param  { list_add($1, $3); $$ = $1; }

param_list: 
    OPENB CLOSEB             { $$ = new_list(); }
|   OPENB param_list1 CLOSEB { $$ = $2; }

opt_func_ret_type:
    /* empty */              { $$ = 0; } 
|   ARROW type               { $$ = $2; }


end_func:
    OUTDENT
|   OUTDENT END EOL
|   OUTDENT END FUN EOL

func_decl: FUN IDENTIFIER param_list opt_func_ret_type EOL block end_func
                            { $$ = new_AST_Function(@1, $2, $3, $4, $6); }

block:
    INDENT                  { $$ = new_block(); }
|   block block_stmt        { block_add_statement($1, $2); $$ = $1; }

block_stmt:
    expr_stmt
|   declaration




%%

void yyerror(const char *s) {
    error(&yylloc, s);
}
