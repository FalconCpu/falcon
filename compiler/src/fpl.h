#include <iostream>
#include <string>
#include <vector>
using std::string;
using std::vector;

// ==============================================================================================
//                                         Lexer
// ==============================================================================================

void open_file(string filename);
int  yylex();
int  yyparse();
string get_token_name(int token);

#define TOKEN_EOF        0x00
#define TOKEN_EOL        0x01
#define TOKEN_INDENT     0x02
#define TOKEN_OUTDENT    0x03
#define TOKEN_IDENTIFIER 0x04
#define TOKEN_INTLIT     0x05
#define TOKEN_REALLIT    0x06
#define TOKEN_STRINGLIT  0x07
#define TOKEN_CHARLIT    0x08
#define TOKEN_PLUS       0x09
#define TOKEN_MINUS      0x0A
#define TOKEN_STAR       0x0B
#define TOKEN_SLASH      0x0C
#define TOKEN_PERCENT    0x0D
#define TOKEN_AMP        0x0E
#define TOKEN_BAR        0x0F
#define TOKEN_CARAT      0x10
#define TOKEN_EQ         0x11
#define TOKEN_NEQ        0x12
#define TOKEN_LT         0x13
#define TOKEN_LTE        0x14
#define TOKEN_GT         0x15
#define TOKEN_GTE        0x16
#define TOKEN_NOT        0x17
#define TOKEN_AND        0x18
#define TOKEN_OR         0x19
#define TOKEN_DOT        0x1A
#define TOKEN_ARROW      0x1B
#define TOKEN_COMMA      0x1C
#define TOKEN_COLON      0x1D
#define TOKEN_OPENB      0x1E
#define TOKEN_OPENSQ     0x1F
#define TOKEN_OPENCL     0x20
#define TOKEN_CLOSEB     0x21
#define TOKEN_CLOSESQ    0x22
#define TOKEN_CLOSECL    0x23
#define TOKEN_VAL        0x24
#define TOKEN_VAR        0x25
#define TOKEN_IF         0x26
#define TOKEN_ELSE       0x27
#define TOKEN_END        0x28
#define TOKEN_WHILE      0x29
#define TOKEN_REPEAT     0x2A
#define TOKEN_UNTIL      0x2B
#define TOKEN_CLASS      0x2C
#define TOKEN_ENUM       0x2D
#define TOKEN_RETURN     0x2E
#define TOKEN_FUN        0x2F
#define TOKEN_ERROR      0x30


// ==============================================================================================
//                                         Location
// ==============================================================================================

struct Location {
    string filename;
    int    line;
    int     column;

    string get_location() const {return filename + ":" + std::to_string(line) + "." + std::to_string(column);}
    Location(string filename, int line, int column) : filename(filename), line(line), column(column) {}    
    Location(const Location& other) : filename(other.filename), line(other.line), column(other.column) {}
    Location() : filename(""), line(0), column(0) {}   

    // Add the copy assignment operator
    Location& operator=(const Location& other) {
        this->filename = other.filename;
        this->line = other.line;
        this->column = other.column;
        return *this;
    } 
};


// ==============================================================================================
//                                         Ast
// ==============================================================================================

class Ast{
    protected:
    Location location;

    public:
    Ast(Location location) : location(location) {}
    virtual void tree_print(int indent) = 0;
};

// ==============================================================================================
//                                         Ast_block
// ==============================================================================================

class Ast_block : public Ast {
    vector<Ast*>     statements;
    Ast_block*       parent;

    public:
    Ast_block(Location location) : Ast(location) {}
    void add(Ast* statement);
    virtual void tree_print(int indent);
};

// ==============================================================================================
//                                         Ast_top
// ==============================================================================================

class Ast_top : public Ast_block {

    public:
    Ast_top() : Ast_block(Location()) {}
};


// ==============================================================================================
//                                         Ast_expression
// ==============================================================================================

class Ast_expression : public Ast {
    public:
    Ast_expression(Location location) : Ast(location) {}
};

// ==============================================================================================
//                                         Ast_identifier
// ==============================================================================================

class Ast_identifier : public Ast_expression {
    string* name;    

    public:
    Ast_identifier(Location location, string* name) : Ast_expression(location), name(name) {}
    void tree_print(int indent);
};

// ==============================================================================================
//                                         Ast_int_literal
// ==============================================================================================

class Ast_intlit : public Ast_expression {
    int value;

    public:
    Ast_intlit(Location location, string* name);
    void tree_print(int indent);
};

// ==============================================================================================
//                                         Ast_binop
// ==============================================================================================

class Ast_binop : public Ast_expression {
    Ast_expression *lhs;
    Ast_expression *rhs;
    int op;

    public:
    Ast_binop(Location location, int op, Ast_expression* lhs, Ast_expression* rhs) : 
        Ast_expression(location), lhs(lhs), rhs(rhs), op(op) {}
    void tree_print(int indent);
};


// ==============================================================================================
//                                         Ast_funccalll
// ==============================================================================================

class Ast_funccall : public Ast_expression {
    Ast_expression*         lhs;
    vector<Ast_expression*> args;

    public:
    Ast_funccall(Location location, Ast_expression* lhs, vector<Ast_expression*> args) :
        Ast_expression(location), lhs(lhs), args(args) {}
    void tree_print(int indent);
};

// ==============================================================================================
//                                         Ast_index
// ==============================================================================================

class Ast_index : public Ast_expression {
    Ast_expression*         lhs;
    Ast_expression*         rhs;

    public:
    Ast_index(Location location, Ast_expression* lhs, Ast_expression* rhs) :
        Ast_expression(location), lhs(lhs), rhs(rhs) {}
    void tree_print(int indent);
};

// ==============================================================================================
//                                         Ast_member
// ==============================================================================================

class Ast_member : public Ast_expression {
    Ast_expression*         lhs;
    const string*           name;

    public:
    Ast_member(Location location, Ast_expression* lhs, const string* name) :
        Ast_expression(location), lhs(lhs), name(name) {}
    void tree_print(int indent);
};

// ==============================================================================================
//                                         Ast_unary
// ==============================================================================================

class Ast_unary : public Ast_expression {
    int                     op;
    Ast_expression*         arg;

    public:
    Ast_unary(Location location, int op, Ast_expression* arg):
        Ast_expression(location), op(op), arg(arg) {}
    void tree_print(int indent);
};







// ==============================================================================================
//                                         Utils
// ==============================================================================================

void fatal(const string message, ...);
void error(const Location& location, const string message, ...);
