
%{
    #include "../src/fpl.h"
    #include <vector>
    using std::vector;
    using std::string;

    #define YYLTYPE Location 

    void yyerror(const char *s);
    int yylex(void);

    Ast_top* ast_top = new Ast_top();

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
%token DEDENT     0x03 "dedent"
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

%error-verbose
%union {
    string*                  string;
    Ast*                     ast;
    Ast_expression*          expr;
    Ast_typeexpr*            type;
    Ast_statement*           stmt;
    Ast_block*               block;
    vector<Ast_expression*>* expr_list;
    vector<Ast_statement*>*  stmt_list;

}

%type <string> IDENTIFIER INTLIT
%type <ast> program  
%type <expr> expression primary postfix prefix mult add comp not and or bracket_expression
%type <expr> opt_eq
%type <expr_list> arg_list expression_list
%type <type> type_expr opt_type
%type <stmt> assignment declaration statement 
%type <block> while1 while


%%

program:
    /* empty */             {$$=0;}
|   program statement       {ast_top->add($2);}

statement:
    assignment              {$$ = $1;}
|   declaration             {$$ = $1;}
|   while                   {$$ = $1;}

bracket_expression:
    OPENB expression CLOSEB {$$ = $2;}

primary:
    bracket_expression
|   IDENTIFIER              {$$ = new Ast_identifier(@1, $1);}
|   INTLIT                  {$$ = new Ast_intlit(@1, $1);}

postfix:
    primary
|   postfix arg_list        {$$ = new Ast_funccall(@1, $1, *$2);}
|   postfix DOT IDENTIFIER  {$$ = new Ast_member(@1, $1, $3);}
|   postfix OPENSQ expression CLOSESQ {$$ = new Ast_index(@1, $1, $3);}

prefix:
    postfix
|   MINUS prefix            {$$ = new Ast_unary(@1, MINUS, $2);}

mult:
    prefix
|   mult STAR prefix        {$$ = new Ast_binop(@1, STAR, $1, $3);}  
|   mult SLASH prefix       {$$ = new Ast_binop(@1, SLASH, $1, $3);}
|   mult PERCENT prefix     {$$ = new Ast_binop(@1, PERCENT, $1, $3);}
|   mult AMP prefix         {$$ = new Ast_binop(@1, AMP, $1, $3);}

add:
    mult
|   add PLUS mult           {$$ = new Ast_binop(@1, PLUS, $1, $3);}
|   add MINUS mult          {$$ = new Ast_binop(@1, MINUS, $1, $3);}
|   add BAR mult            {$$ = new Ast_binop(@1, BAR, $1, $3);}
|   add CARAT mult          {$$ = new Ast_binop(@1, CARAT, $1, $3);}

comp:
    add
|   add EQ add              {$$ = new Ast_binop(@1, EQ, $1, $3);}
|   add NEQ add             {$$ = new Ast_binop(@1, NEQ, $1, $3);}
|   add LT add              {$$ = new Ast_binop(@1, LT, $1, $3);}
|   add LTE add             {$$ = new Ast_binop(@1, LTE, $1, $3);}
|   add GT add              {$$ = new Ast_binop(@1, GT, $1, $3);}
|   add GTE

not:
    comp
|   NOT not                 {$$ = new Ast_unary(@1, NOT, $2);}

and:
    not
|   and AND not             {$$ = new Ast_binop(@1, AND, $1, $3);}

or:
    and
|   or OR and               {$$ = new Ast_binop(@1, OR, $1, $3);}


expression:
    or

arg_list:
    OPENB CLOSEB            {$$ = new vector<Ast_expression*>();}
|   OPENB expression_list CLOSEB {$$ = $2;}

expression_list:
    expression              {$$ = new vector<Ast_expression*>(); $$->push_back($1);}
|   expression_list COMMA expression 
                            {$$ = $1; $$->push_back($3);}

assignment:
    postfix EQ expression EOL {$$ = new Ast_assign(@1, $1, $3);}

type_expr:
    IDENTIFIER              {$$ = 0;}  // TODO

opt_type:
    /* empty */             {$$ = 0;}
|   COLON type_expr         {$$ = $2;}

opt_eq:
    /* empty */             {$$ = 0;}
|   EQ expression           {$$ = $2;}

declaration:
    VAL IDENTIFIER opt_type opt_eq EOL    {$$ = new Ast_declaration(@1, VAL, $2, $3, $4);}
|   VAR IDENTIFIER opt_type opt_eq EOL    {$$ = new Ast_declaration(@1, VAR, $2, $3, $4);}

while1:
    WHILE expression EOL INDENT    {$$ = new Ast_while(@1, $2);}
|   while1 statement               {$$ = $1; $$->add($2);}

while:
    while1 DEDENT
|   while1 DEDENT END EOL
|   while1 DEDENT END WHILE EOL


%%

void yyerror(const char *s) {
    error(yylloc, s);
}
