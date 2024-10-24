#ifndef FPL_H
#define FPL_H

typedef const char* String;
typedef struct Location Location;
typedef struct Type* Type;
typedef struct AST* AST;
typedef struct Block* Block;
typedef struct Symbol* Symbol;
typedef struct AST_list* AST_list;

// =====================================================
//                    Location
// =====================================================

struct Location {
    String filename;
    int line;
    int column;
};


// =====================================================
//                    Token
// =====================================================

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

// =====================================================
//                    Type
// =====================================================

struct Type {
    int kind;
    String name;
};

#define TYPE_UNIT       0x200
#define TYPE_NULL       0x201
#define TYPE_NOTHING    0x202
#define TYPE_BOOL       0x203
#define TYPE_CHAR       0x204
#define TYPE_INT        0x205
#define TYPE_REAL       0x206
#define TYPE_STRING     0x207
#define TYPE_ARRAY      0x208
#define TYPE_FUNCTION   0x209
#define TYPE_CLASS      0x20A
#define TYPE_ENUM       0x20B

extern Type type_unit;
extern Type type_null;
extern Type type_nothing;
extern Type type_bool;
extern Type type_char;
extern Type type_int;
extern Type type_real;
extern Type type_string;
extern Block predefined_names;

void initialize_types();
Type get_type_from_code(int code);

// =====================================================
//                    Symbol
// =====================================================

struct Symbol {
    int    kind;
    String name;
    Type   type;
};

#define SYMBOL_CONSTANT 0x300
#define SYMBOL_LOCAL    0x301
#define SYMBOL_GLOBAL   0x302
#define SYMBOL_FUNCTION 0x303

Symbol new_symbol_local(String name, Type type, int mutable);
Symbol new_symbol_global(String name, Type type, int mutable);

// =====================================================
//                    AST
// =====================================================

struct AST {
    Location location;
    int      kind;
    Type     type;
};

#define AST_TOP          0x100
#define AST_INT_LITERAL  0x101
#define AST_IDENTIFIER   0x102
#define AST_BINOP        0x103
#define AST_DECLARATION  0x104
#define AST_TYPENAME     0x105
#define AST_PARAMETER    0x106
#define AST_FUNCTION     0x107

void print_ast(AST n, int indent);
void type_check(AST n, Block context);
void build_hierarchy(AST n, Block context);

typedef struct AST_Top* AST_Top;
AST            new_ast_top();
AST_IntLiteral as_Top(AST n);
void           print_ast_Top(AST_Top n, int indent);
void           type_check_top(AST_Top n, Block context);

typedef struct AST_IntLiteral* AST_IntLiteral;
AST            new_ast_IntLiteral(Location location, int value);
AST_IntLiteral as_IntLiteral(AST n);
void           print_ast_IntLiteral(AST_IntLiteral n, int indent);
void           type_check_IntLiteral(AST_IntLiteral n, Block context);
int            stringToInt(String s);

typedef struct AST_Identifier* AST_Identifier;
AST            new_ast_Identifier(Location location, String name);
AST_Identifier as_Identifier(AST n);
void           print_ast_Identifier(AST_Identifier n, int indent);
void           type_check_Identifier(AST_Identifier n, Block context);

typedef struct AST_Binop* AST_Binop;
AST            new_ast_Binop(Location location, int op, AST lhs, AST rhs);
AST_Binop      as_Binop(AST n);
void           type_check_Binop(AST_Binop n, Block context);
void           print_ast_Binop(AST_Binop n, int indent);

typedef struct  AST_Declaration* AST_Declaration;
AST             new_ast_Declaration(Location location, int op, String name, AST ast_type, AST value);
AST_Declaration as_Declaration(AST n);
void            type_check_Declaration(AST_Declaration n, Block context);
void            print_ast_Declaration(AST_Declaration n, int indent);

typedef struct  AST_TypeName* AST_TypeName;
AST             new_ast_TypeName(Location location, String name);
AST_TypeName    as_TypeName(AST n);
void            print_ast_TypeName(AST_TypeName n, int indent);

typedef struct  AST_Parameter* AST_Parameter;
AST             new_ast_Parameter(Location location, int op, String name, AST ast_type);
AST_Parameter   as_Parameter(AST n);
void            print_ast_Parameter(AST_Parameter n, int indent);

typedef struct  AST_Function* AST_Function;
AST             new_AST_Function(Location location, String name, AST_list parameters, AST ast_return_type, Block body);
AST_Function    as_Function(AST n);
void            type_check_Function(AST_Function n, Block context);
void            print_ast_Function(AST_Function n, int indent);
void            build_hierarchy_function(AST_Function n, Block context);

// =====================================================
//                    AST_List
// =====================================================

typedef struct AST_list* AST_list;
struct AST_list {
    int num_elements;
    int alloc_elements;
    AST* elements;
};

AST_list new_list();
void list_add(AST_list l, AST e);
AST list_get(AST_list l, int index);
#define foreach(L, I) for(int list_index=0; (I = (list_index<(L)->num_elements ? (L)->elements[list_index] : 0)); list_index++) 


// =====================================================
//                    Block
// =====================================================

struct Block {
    Location location;
    int      kind;
    Type     type;

    Block parent;

    int num_statements;
    int alloc_statements;
    AST* statements;

    int num_symbols;
    int alloc_symbols;
    Symbol* symbols;

    int num_types;
    int alloc_types;
    Type* types;
};

Block           as_block(AST this);
void            block_add_statement(Block this, AST statement);
void            block_add_symbol(Block this, Symbol s);
void            block_add_type(Block this, Type t);
Symbol          block_lookup_symbol(Block this, String name);
Type            block_lookup_type(Block this, String name);
void            build_hierarchy_block(Block this, Block parent);

#define foreach_statement(B, S) for(int stmt_index=0; (S = (stmt_index<(B)->num_statements ? (B)->statements[stmt_index] : 0)); stmt_index++) 
#define foreach_symbol(B, S) for(int sym_index=0; (S = (sym_index<(B)->num_symbols ? (B)->symbols[sym_index] : 0)); sym_index++)
#define foreach_type(B, T) for(int type_index=0; (T = (type_index<(B)->num_types ? (B)->types[type_index] : 0)); type_index++)




// =====================================================
//                    Lexer
// =====================================================

typedef struct {
    String    key;
    int       kind;
} HashEntry;

void open_file(String filename);
int yylex();
String token_kind_name(int kind);
HashEntry* hash_table_add(String s, int kind);

int yyparse();

// =====================================================
//                    Utils
// =====================================================

void  fatal(String message, ...) __attribute((noreturn));  
void  error(Location* location, String message, ...);
void* my_malloc(int size);
void* my_realloc(void* ptr, int size);
void  my_free(void* ptr);
#define new(T) ((T*)my_malloc(sizeof(T)))
#define new_array(T, n) ((T*)my_malloc(sizeof(T) * n))
#define resize_array(A, T, N) (A)=((T*)my_realloc(A, sizeof(T) * (N)))
#define assert(cond) if (!(cond)) fatal("Assertion failed: %s at %s.%d", #cond, __FILE__, __LINE__)
#define get_location(T) ((Location*)T)
#define TODO() fatal("Not yet implemented at %s %d", __FILE__, __LINE__)

#endif // FPL_H
