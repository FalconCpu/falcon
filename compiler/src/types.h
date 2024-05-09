#include "fpl.h"

// =================================================================================
//                                Type
// =================================================================================
// Types are represented as a Type Structure
// A single instance of the type structure is created for every primative type at startup
//
// More complex types have an extended version of the type struct with fields relevant
// to their type. When the type checker comes across a type it has not yet encountered
// a new instance of the type structure will be created and stored in a list. Later
// references to the same type will point to the same instance.
// Thus we can check two expressions are of the same type by simply comparing type pointers.
// (Note this may change if we add sub-classes later. So all type comparisions should be
// done using the check_type_compatible function).

typedef struct Type* Type;
typedef struct Type_struct* Type_struct;
typedef struct TypeList* TypeList;


struct Type_array {
    int    kind;   // will be TYPE_ARRAY
    String name;
    Type   base;   // The type of the elements in this array
    int    size;   // Number of elements, 0 if unknown
};

struct Type_pointer {
    int    kind;   // will be TYPE_POINTER
    String name;
    Type   base;     // The type pointed to
    int    nullable; // 1 to indicate this pointer is nullable
};

struct Type_function {
    int    kind;   // will be TYPE_FUNCTION
    String name;
    Type   return_type;
    TypeList parameter_types;
};

struct Type_struct {
    int        kind;   // will be TYPE_STRUCT
    String     name;
    SymbolList params;  // the fields to be populated by the constructor
    Block      body;    // the body of the struct
    int        size;    // calculated during type checking
    Symbol     symbol;  // Symbol for the type descriptor
};

// primative types
void initialize_types();   // initializes the type system, and instantiates the primative types
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
extern Type type_voidstar;

// safe casts
struct Type_array*    as_TypeArray(Type t);
struct Type_pointer*  as_TypePointer(Type t);
struct Type_function* as_TypeFunction(Type t);
struct Type_struct*   as_TypeStruct(Type t);




// Structure to represent a list of types
struct TypeList {
    int count;
    int alloc;
    Type* types;
};
TypeList new_TypeList();
void TypeList_add(TypeList list, Type item);
Type TypeList_get(TypeList list, int index);
#define foreach_type(A,T,L) for(int index=0; ((A=(T)(TypeList_get(L,index)))); index++)
