#ifndef FPL_H
#define FPL_H

typedef const char* String;
typedef struct Location Location;
typedef struct Type* Type;
typedef struct AST* AST;


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
#define TOKEN_ERROR      0x2F

// =====================================================
//                    Type
// =====================================================

struct Type {
    int kind;
    String name;
};


// =====================================================
//                    Compiler Stop At
// =====================================================

typedef enum CompilerStopAt {
    STOP_AT_NONE,
    STOP_AT_LEXER,
    STOP_AT_AST,
    STOP_AT_IR,
    STOP_AT_ASM
} CompilerStopAt;

// =====================================================
//                    Lexer
// =====================================================

void open_file(String filename);
int yylex();
String token_kind_name(int kind);

int yyparse();

// =====================================================
//                    Utils
// =====================================================

void  fatal(String message, ...);  
void  error(Location* location, String message, ...);
void* my_malloc(int size);
void* my_realloc(void* ptr, int size);
void  my_free(void* ptr);
#define new(T) ((T*)my_malloc(sizeof(T)))
#define new_array(T, n) ((T*)my_malloc(sizeof(T) * n))
#define assert(cond) if (!(cond)) fatal("Assertion failed: %s at %s.%d", #cond, __FILE__, __LINE__)

#endif // FPL_H
