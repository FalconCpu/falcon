#include "fpl.h"

#define AST_INT_LITERAL  0x100
#define AST_IDENTIFIER   0x101
#define AST_BINOP        0x102

struct AST {
    Location location;
    int      kind;
    Type     type;
};

void print_ast(AST n, int indent);

// ====================================================
//                    Literal
// ====================================================

typedef struct AST_IntLiteral* AST_IntLiteral;
struct AST_IntLiteral {
    Location location;
    int      kind;
    Type     type;

    int value;
};

AST new_ast_IntLiteral(Location location, int value);
AST_IntLiteral as_IntLiteral(AST n);
void print_ast_IntLiteral(AST_IntLiteral n, int indent);
int stringToInt(String s);

// ====================================================
//                    Identifier
// ====================================================

typedef struct AST_Identifier* AST_Identifier;
struct AST_Identifier {
    Location location;
    int      kind;
    Type     type;

    String   name;
};

AST new_ast_Identifier(Location location, String name);
AST_Identifier as_Identifier(AST n);
void print_ast_Identifier(AST_Identifier n, int indent);

// ====================================================
//                    Binary Operator
// ====================================================

typedef struct AST_Binop* AST_Binop;
struct AST_Binop {
    Location location;
    int      kind;
    Type     type;

    int      op;
    AST      lhs;
    AST      rhs;
};

AST new_ast_Binop(Location location, int op, AST lhs, AST rhs);
AST_Binop as_Binop(AST n);
void print_ast_Binop(AST_Binop n, int indent);






