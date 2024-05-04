#ifndef FPL_H
#define FPL_H

// ===========================================================================
//                    Main types used in the program
// ===========================================================================

typedef const char* String;
typedef struct Location* Location;
typedef struct Token* Token;
typedef struct AST* AST;
typedef struct Type* Type;
typedef struct Block* Block;
typedef struct Symbol* Symbol;
typedef struct SymbolList* SymbolList;
typedef struct AST_list* AST_list;
typedef struct Function* Function;
typedef struct TypeList* TypeList;
typedef enum TokenKind TokenKind;
typedef enum BinopKind BinopKind;
typedef enum InstrKind InstrKind;
typedef enum AluOp AluOp;
typedef struct Instr* Instr;
typedef union Value Value;
typedef struct Bitmap* Bitmap;

// ===========================================================================
//                    Types
// ===========================================================================

typedef enum {
    TYPE_VOID,
    TYPE_NULL,
    TYPE_BOOL,
    TYPE_CHAR,
    TYPE_SHORT,
    TYPE_INT,
    TYPE_LONG,
    TYPE_REAL,
    TYPE_STRING,
    TYPE_ARRAY,
    TYPE_POINTER,
    TYPE_FUNCTION,
    TYPE_STRUCT,
    TYPE_ERROR
} TypeKind ;


struct Type {
    TypeKind kind;   // See list below
    String   name;
};

// generate types:-
Type make_type_error(Location location, String Message, ...);
Type make_type_pointer(Type base, int nullable);
Type make_type_function(TypeList parameter_type, Type return_type);
Type make_type_array(Type base, int size);
Type make_type_struct(String name);
Type get_type_from_code(int code);

// Checks to see if an expression is assignment compatible with a given type
// if not report an error
void check_type_compatible(Type type, AST expression);

// gets the size of a type
int get_sizeof(Type t);

// gets the size of a type
int is_scalar_type(Type t);

// Run the typechecker
void typecheck(Block top);

// the primative types
extern Type type_void;
extern Type type_null;
extern Type type_bool;
extern Type type_char;
extern Type type_short;
extern Type type_int;
extern Type type_long;
extern Type type_real;
extern Type type_string;
extern Type type_error;

void initialize_types();

void AST_typecheck_value(AST this, Block scope);

int is_type_expr(AST this);

// ---------------------------------------------------------------------------------
//                         Value
// ---------------------------------------------------------------------------------

union Value {
    int      int_val;
    Function function;
    String   str_val;
};


// ---------------------------------------------------------------------------------
//                         Symbols
// ---------------------------------------------------------------------------------
// Symbols are used to represent anything that can be searched for by name
// primarily variables, but also constants, functions, and types.

typedef enum {
    SYM_CONSTANT,
    SYM_VARIABLE,
    SYM_FUNCTION,
    SYM_TYPEREF,
    SYM_GLOBALVAR,
    SYM_AGGREGATE,
    SYM_LABEL,
    SYM_REG
} SymbolKind;

struct Symbol {
    Location   location; // Textual location where the symbol is defined, for use in error messages
    SymbolKind kind;     // See list below
    String     name;     // Name of symbol
    Type       type;     // Symbols Type
    int        mutable;
    Value      value;    // For constants and functions this points to the AST where the value is defined
    int        offset;   // For struct members contains the offset

    // fields used during optimisation
    int        read_count;
    int        write_count;
    int        index;
    Symbol     copy_of;     // set in peephole to indicate this symbol is a copy of another
    int        reg_number;
};

extern String symbol_kind_name[];

Symbol new_Symbol(Location loc, int kind, String name, Type type, int mutable);
String Symbol_toText(Symbol this);
Symbol get_aggregate_address(Function func, Symbol this);

// ---------------------------------------------------------------------------------
//                     Symbol List
// ---------------------------------------------------------------------------------
// Structure to represent a list of symbols

struct SymbolList {
    int     count;
    int     alloc;
    Symbol* symbols;
};
SymbolList new_SymbolList();
void       SymbolList_add(SymbolList list, Symbol item);
Symbol     SymbolList_get(SymbolList list, int index);
Symbol     SymbolList_find(SymbolList list, String name);
#define foreach_symbol(N,L) for(int index=0; ((N=SymbolList_get(L,index))); index++)


// ===========================================================================
//                    AST
// ===========================================================================
// Abstract Syntax Tree - subclasses defined in AST.h

typedef enum  {
    AST_INTLIT,
    AST_STRLIT,
    AST_SYMBOL,
    AST_BINOP,
    AST_UNARY,
    AST_INDEX,
    AST_POINTER,
    AST_MEMBER,
    AST_FUNCCALL,
    AST_DECL,
    AST_RETURN,
    AST_ASSIGN,
    AST_WHILE,
    AST_REPEAT,
    AST_IF,
    AST_CLAUSE,
    AST_FUNCTION,
    AST_STRUCT
} ASTkind;

struct AST {
    Location location;
    ASTkind  kind;
    Type     type;
};

AST new_AST_intlit(Location location, Type type, int value);
AST new_AST_strlit(Location location, Type type, String value);
AST new_AST_symbol(Location location, String name);
AST new_ASTnode_id_from_symbol(Symbol sym);
AST new_AST_binop(Location location, TokenKind op, AST lhs, AST rhs);
AST new_AST_unary(Location location, TokenKind op, AST rhs);
AST new_AST_index(Location location,  AST lhs, AST rhs);
AST new_AST_pointer(Location location,  AST rhs);
AST new_AST_member(Location location,  AST lhs, String name, int null_coalese);
AST new_AST_funccall(Location location,  AST lhs, AST_list rhs);
AST new_AST_decl(Location location, String name, AST ast_type, AST ast_value, int mutable);
AST new_AST_return(Location location, AST retval);
AST new_AST_assign(Location location, AST lhs, AST rhs);
AST new_AST_while(Location location, AST condition, Block body);
AST new_AST_repeat(Location location, AST condition, Block body);
AST new_AST_if(Location location, Block clauses);
AST new_AST_clause(Location location, AST condition, Block body);
AST new_AST_struct(Location location, String name, AST_list members);


void  AST_print(AST this, int indent);

Block AST_get_block(AST this);

// Test to see if an expression is an lvalue
void check_lvalue(AST this);

// Determines if an AST has a known value, if so reports it
// if not give an error message and return zero
int AST_get_constant_value(AST this);

// runs the typechecking phase on any kind of AST node
void AST_typecheck(AST this, Block scope);

Symbol code_gen(Function func, AST this);

Symbol code_gen_(Function func, AST this);
void   code_gen_lvalue(Function func, AST this, Symbol value);
Symbol code_gen_aggregate_lhs(Function func, AST this);
void   code_gen_aggregate_rhs(Function func, AST this, Symbol lhs);
void   code_gen_bool(Function func, AST this, Symbol lab_true, Symbol lab_false);


#define as_AST(A) ((AST)(A))




// ===========================================================================
//                    AST_list
// ===========================================================================
// A list of AST nodes

struct AST_list {
    int count;
    int alloc;
    AST* items;
};


AST_list new_AST_list();
void     AST_list_add(AST_list list, AST item);
AST      AST_list_get(AST_list list, int index);
#define foreach(A,L) for(int index=0; (((A)=AST_list_get(L,index))); index++)


// ===========================================================================
//                      Block
// ===========================================================================
// Reprsents a block of code

struct Block {
    Block   parent;
    int     num_statements;
    int     alloc_statements;
    AST*    statements;

    AST        enclosing_statement;
    SymbolList symbols;
};

Block  new_block(Block parent);
void   block_add_statement(Block this, AST statement);
AST    block_get_statement(Block this, int index);
void   block_print(Block this, int indent);
void   block_add_symbol(Block block, Symbol sym);
Symbol block_find_symbol(Block block, String name);
Function block_find_enclosing_function(Block this);
Symbol code_gen_block(Function func, Block this);

#define foreach_statement(A,L)  for(int index=0; (((A)=block_get_statement(L,index))); index++)

// ===========================================================================
//                    Function
// ===========================================================================
// Represents a function

struct Function {
    Location location;
    ASTkind  kind;
    Type     type;          // Filled in at type checking. Will be Type_function

    String   name;
    AST_list params;        // AST for each parameter
    AST      ret_type_ast;
    Block    body;

    // filled in at type checking
    SymbolList parameters;  // Symbol for each parameter
    Type       return_type;

    // for codegen
    SymbolList  all_vars;          // Flattened list of all variables (including temporaries)
    SymbolList  all_consts;        // All Constant values used
    SymbolList  all_labels;        // All labels in the block
    int         num_instr;
    int         alloc_instr;
    Instr*      instr;             // The instructions that make up the program
    int         num_temps;         // number of temporary variables
    Symbol      end_label;
    Bitmap      livemap;            // which variables are live at each instruction. Set during peephole()
    int         max_reg_no;         // maximum register number used
    int         makes_calls;        // set to 1 if the block contains a call instruction (set in regalloc)
    int         stack_vars;         // amount of stack space needed for variables
};

AST new_function(Location location, String name, AST_list params, AST ret_type_ast, Block body);

Function as_function(AST this);

// Add an instuction to the end of the program
void add_instr(Function this, Instr inst);

// create a new tempvar symbol
Symbol new_tempvar(Function this, Type type);

// check the given symbol is in the post codegen symbol table
Symbol get_codegen_symbol(Function this, Symbol sym);

// Find a symbol relating to a given constant
Symbol make_constant_symbol(Function this, int value);

// Find a symbol relating to a given string literal
Symbol make_string_constant_symbol(Function this, String value);

void print_program(Function this);
void print_program_with_bitmap(Function this, Bitmap bm1, Bitmap bm2);

// create a new label
Symbol new_label(Function this);


Symbol code_gen(Function func, AST this);
Symbol code_gen_function(Function this);
void code_gen_lvalue(Function func, AST this, Symbol value);
void code_gen_aggregate_rhs(Function func, AST this, Symbol value);
Symbol code_gen_aggregate_lhs(Function func, AST this);
void code_gen_bool(Function func, AST this, Symbol lab_true, Symbol lab_false);

Function get_function_by_index(int index);
#define foreach_function(F) for(int index=0; ((F=get_function_by_index(index))); index++)

// ===========================================================================
//                instr.c
// ===========================================================================

Instr new_Instr(InstrKind kind, AluOp op, Symbol dest, Symbol a, Symbol b);

#define foreach_instr(I,P) for(int index=0; index<P->num_instr ? (I=P->instr[index]) : 0; index++)

// ===========================================================================
//                parse.c
// ===========================================================================

void parse_top(Block block);

// ===========================================================================
//                      Stdlib
// ===========================================================================
// Code to emit Standard library functions

struct Stdlib {
    Symbol sdlib_malloc;
    Symbol sdlib_bzero;
    Symbol sdlib_memcpy;
};

extern struct Stdlib stdlib;
void initialize_stdlib();

void gen_memcopy(Function func, Symbol dest, Symbol source, int size);

// =================================================================================
//                                bitmap.c
// =================================================================================

typedef unsigned long long Ull;

struct Bitmap {
    int num_rows;
    int num_cols;
    int row_shift;
    Ull* data;
};

// Create a new bitmap
Bitmap new_Bitmap(int num_rows, int num_cols);

// Set a bit in a bitmap
void   bitmap_set(Bitmap bm, int row, int col);

// Set a bit in a bitmap
void   bitmap_clear(Bitmap bm, int row, int col);

// Get a bit from a bitmap
int    bitmap_get(Bitmap bm, int row, int col);

// OR a row in one bitmap with a row in another bitmap
// Returns 1 if any new bits are set in the destination bitmap
int    bitmap_or_row(Bitmap dest_bm, int dest_row, Bitmap source_bm, int source_row);

// combines the data in a row of the destination bitmap with a row from bitmap A AND'ed with a row from bitmap B
void  bitmap_or_and_not_row(Bitmap dest_bm, int dest_row, Bitmap bm_a, int row_a, Bitmap bm_b, int row_b);

void delete_Bitmap(Bitmap bm);


// ===========================================================================
//                Common utility functions - in util.c
// ===========================================================================

// Abort the program with an error message
void fatal(String message, ...) __attribute((noreturn));

// Report an error to the user  (with location if availible)
void error(Location loc, String message, ...);

// Report a warning to the user (with location if availible)
void warn(Location loc, String message, ...);

// Allocate and clear a block of memory. Abort program if allocation fails.
void* my_malloc(int size);

// realloc() a block of memory. Abort program if allocation fails.
void* my_realloc(void *old, int new_size);

// Allocate a memory for a new string, and concatenate several strings into it
String my_strcat2(String s1, String s2);
String my_strcat3(String s1, String s2, String s3);
String my_strcat4(String s1, String s2, String s3, String s4);

// print spaces, used to print aligned indent code
void print_spaces(int indent);

// Allocate memory for a structure, and typecast pointer
#define new(T) (T*)(my_malloc(sizeof(T)))

// Allocate memory for an array, and typecast pointer
#define new_array(T,N) (T*)(my_malloc((N)*sizeof(T)))

// Realloc() memory for an array
#define resize_array(A,T,N) A = (T*)my_realloc(A, (N)*sizeof(T))

// Abort program if a condition is not met (to be used to check internal state consistency only!)
#define assert(A) if(!(A)) fatal("Assertion '%s' failed at %s.%d", #A, __FILE__, __LINE__)

// Abort program on reaching unimplemented code
#define TODO(A) fatal("Not yet implemented %s at %s %d",#A, __FILE__,__LINE__)

// Get a unique pointer for a given string value. Allows string comparisons to be done by
// pointer comparisoon, not requiring strcmp
String unique_string(String s);

String location_toString(Location location);


#endif
