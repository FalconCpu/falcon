#include <stdio.h>
#include "fpl.h"
#include "types.h"

Type type_void;
Type type_null;
Type type_bool;
Type type_char;
Type type_short;
Type type_int;
Type type_long;
Type type_real;
Type type_string;
Type type_error;
Type type_voidstar;

SymbolList builtin_types;


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

static TypeList all_pointer_types;
static TypeList all_function_types;
static TypeList all_array_types;
static TypeList all_struct_types;

// Create a primative type,and add it to the table of builin types
static Type make_primative_type(int kind, String name) {
    Type ret = new(struct Type);
    ret->kind = kind;
    ret->name = name;

    SymbolList_add(builtin_types, new_Symbol(0, SYM_TYPEREF, name, ret, 0));
    return ret;
}

// --------------------------------------------------------------------------------
//               initialize_types
// ---------------------------------------------------------------------------------
// create the primative types, and initialize the type pools for the complex types

void initialize_types() {
    if (builtin_types!=0)
        return; // Already been called

    // create the primative types.
    builtin_types = new_SymbolList();
    type_void    = make_primative_type(TYPE_VOID,   unique_string("Void"));
    type_null    = make_primative_type(TYPE_NULL,   unique_string("Null"));
    type_bool    = make_primative_type(TYPE_BOOL,   unique_string("Bool"));
    type_char    = make_primative_type(TYPE_CHAR,   unique_string("Char"));
    type_short   = make_primative_type(TYPE_SHORT,  unique_string("Short"));
    type_int     = make_primative_type(TYPE_INT,    unique_string("Int"));
    type_long    = make_primative_type(TYPE_LONG,   unique_string("Long"));
    type_real    = make_primative_type(TYPE_REAL,   unique_string("Real"));
    type_string  = make_primative_type(TYPE_STRING, unique_string("String"));
    type_error   = make_primative_type(TYPE_STRING, unique_string("<Error>"));

    // create the type pools
    all_pointer_types = new_TypeList();
    all_function_types = new_TypeList();
    all_array_types = new_TypeList();
    all_struct_types = new_TypeList();

    type_voidstar = make_type_pointer(type_void, 0);

    // add symbols for known constants
    Symbol sym_null = new_Symbol(0, SYM_CONSTANT, unique_string("null"), type_null, 0);
    sym_null->value.int_val = 0;
    SymbolList_add(builtin_types, sym_null);
    
    Symbol sym_false = new_Symbol(0, SYM_CONSTANT, unique_string("false"), type_bool, 0);
    sym_false->value.int_val = 0;
    SymbolList_add(builtin_types, sym_false);
    
    Symbol sym_true = new_Symbol(0, SYM_CONSTANT, unique_string("true"), type_bool, 0);
    sym_true->value.int_val = 1;
    SymbolList_add(builtin_types, sym_true);

    initialize_stdlib();
}

// --------------------------------------------------------------------------------
//               Type_pointer
// ---------------------------------------------------------------------------------

Type make_type_pointer(Type base, int nullable) {
    struct Type_pointer* t;
    foreach_type(t, struct Type_pointer*, all_pointer_types)
        if (t->base==base && t->nullable==nullable)
            return (Type)t;

    t = new(struct Type_pointer);
    t->name = my_strcat2(base->name, nullable ? "?" : "*");
    t->kind = TYPE_POINTER;
    t->base = base;
    t->nullable = nullable;
    TypeList_add(all_pointer_types, (Type)t);
    return (Type) t;
}

struct Type_pointer* as_TypePointer(Type t) {
    if (t->kind!=TYPE_POINTER)
        fatal("Attempt to cast %s to Type_pointer", t->name);
    return (struct Type_pointer*)t;
}


// --------------------------------------------------------------------------------
//               compare_type_lists
// ---------------------------------------------------------------------------------

static int compare_type_lists(TypeList list1, TypeList list2) {
    // Check if the lists have the same number of elements
    if (list1->count != list2->count)
        return 0;

    // Iterate through the lists and compare elements
    for (int i = 0; i < list1->count; i++) {
        Type type1 = TypeList_get(list1, i);
        Type type2 = TypeList_get(list2, i);
        if (type1!=type2)
            return 0;
    }

    // If all elements match, return true
    return 1;
}

// --------------------------------------------------------------------------------
//               make_type_function
// ---------------------------------------------------------------------------------

Type make_type_function(TypeList parameter_types, Type return_type) {
    struct Type_function* t;
    foreach_type(t, struct Type_function*, all_function_types)
        if (t->return_type==return_type && compare_type_lists(parameter_types, t->parameter_types) )
            return (Type)t;

    // detemine the name for the new type
    Type param;
    String name = "(";
    foreach_type(param, Type, parameter_types)
        name = my_strcat3( name, index==0 ? "" : ",", param->name);
    name = my_strcat3(name, ")->", return_type->name);

    t = new(struct Type_function);
    t->parameter_types = parameter_types;
    t->return_type = return_type;
    t->kind = TYPE_FUNCTION;
    t->name = name;
    TypeList_add(all_function_types, (Type)t);
    return (Type) t;
}

struct Type_function* as_TypeFunction(Type t) {
    if (t->kind!=TYPE_FUNCTION)
        fatal("Attempt to cast %s to Type_function", t->name);
    return (struct Type_function*)t;
}

// --------------------------------------------------------------------------------
//               make_type_array
// ---------------------------------------------------------------------------------

Type make_type_array(Type base, int size) {
    struct Type_array* t;
    foreach_type(t, struct Type_array*, all_array_types)
        if (t->base==base && t->size==size)
            return (Type)t;

    t = new(struct Type_array);
    if (size==0)
        t->name = my_strcat2(base->name, "[]");
    else {
        char buf[16];
        sprintf(buf,"%d", size);
        t->name = my_strcat4(base->name, "[", buf, "]");
    }
    t->kind = TYPE_ARRAY;
    t->base = base;
    t->size = size;
    TypeList_add(all_array_types, (Type)t);
    return (Type) t;
}

struct Type_array* as_TypeArray(Type t) {
    if (t->kind!=TYPE_ARRAY)
        fatal("Attempt to cast %s to Type_array", t->name);
    return (struct Type_array*)t;
}


// --------------------------------------------------------------------------------
//               make_type_struct
// ---------------------------------------------------------------------------------

Type make_type_struct(String name, Block body) {
    struct Type_struct* ret = new(struct Type_struct);
    ret->name = name;
    ret->kind = TYPE_STRUCT;
    ret->params = new_SymbolList();
    ret->body = body;

    TypeList_add(all_struct_types, (Type) ret);
    return (Type)ret;
}

struct Type_struct* as_TypeStruct(Type t) {
    if (t->kind!=TYPE_STRUCT)
        fatal("Attempt to cast %s to Type_struct", t->name);
    return (struct Type_struct*)t;
}

void output_type_descriptors(FILE *fh) {
    Type_struct t;
    foreach_type(t, Type_struct, all_struct_types) {
        fprintf(fh, "%s:\n", t->name);
        fprintf(fh,"dcw %d\n", t->size);
        fprintf(fh,"dcw \"%s\"\n", t->name);
    }
}

// ------------------------------------------------------------------------------
//                      check_type_compatible
// ------------------------------------------------------------------------------
// Checks to see if an expression is assignment compatible to a type.

void check_type_compatible(Type type, AST expression) {
    // if either side is a type_error then silently ignore the checking
    // to avoid an avalnche of error messages propagating
    if (type==type_error || expression->type==type_error)
        return;

    // allow a non nullable pointer to be assigned to a nullable pointer of the same type
    if (type->kind==TYPE_POINTER && expression->type->kind==TYPE_POINTER) {
        struct Type_pointer* type_p = (struct Type_pointer*) type;
        struct Type_pointer* expr_p = (struct Type_pointer*) expression->type;
        if (type_p->base == expr_p->base && type_p->nullable==1 && expr_p->nullable==0)
            return;
    }

    // allow assigning null to a  nullable pointer
    if (type->kind==TYPE_POINTER && expression->type==type_null) // && as_TypePointer(type)->nullable)
        return;

    // Until we implement sub-types, we can check for type compatibilty by simply
    // comparing pointers
    if (expression->type != type)
        error(expression->location, "Got type '%s' when expecting type '%s'", expression->type->name, type->name);
}


// ------------------------------------------------------------------------------
//                      TypeList
// ------------------------------------------------------------------------------
// Structure to represent a list of types

TypeList new_TypeList() {
    TypeList ret = new(struct TypeList);
    ret->alloc = 4;
    ret->count = 0;
    ret->types = new_array(Type, ret->alloc);
    return ret;
}

void TypeList_add(TypeList list, Type item) {
    if (list==0)
        fatal("Null pointer in TypeList_get");
    if (item==0)
        return;
    if (list->alloc==list->count) {
        list->alloc *= 2;
        resize_array(list->types, Type, list->alloc);
    }
    list->types[list->count++] = item;
}

Type TypeList_get(TypeList list, int index) {
    if (list==0)
        fatal("Null pointer in TypeList_get");
    if (index<0 || index>list->count)
        fatal("Index %d out of bounds in TypeList_get",index);
    if (index==list->count)
        return 0;
    else
        return list->types[index];
}

// ------------------------------------------------------------------------------
//                      get_type_from_code
// ------------------------------------------------------------------------------
// Returns a pointer to a type instance based on an integer code

Type get_type_from_code(int code) {
    switch(code) {
        case TYPE_VOID:     return type_void;
        case TYPE_NULL:     return type_null;
        case TYPE_BOOL:     return type_bool;
        case TYPE_CHAR:     return type_char;
        case TYPE_SHORT:    return type_short;
        case TYPE_INT:      return type_int;
        case TYPE_LONG:     return type_long;
        case TYPE_REAL:     return type_real;
        case TYPE_STRING:   return type_string;
        default:            fatal("Attempt to get_type_from_code %d",code);
    }
}

// --------------------------------------------------------------------------------
//               get_sizeof
// ---------------------------------------------------------------------------------
// calculate the size of a type

int get_sizeof(Type t) {
    switch(t->kind) {
        case TYPE_VOID:     return 0;
        case TYPE_NULL:     return 4;
        case TYPE_BOOL:     return 1;
        case TYPE_CHAR:     return 1;
        case TYPE_SHORT:    return 2;
        case TYPE_INT:      return 4;
        case TYPE_LONG:     return 8;
        case TYPE_REAL:     return 4;
        case TYPE_STRING:   return 4;
        case TYPE_ARRAY:    return as_TypeArray(t)->size * get_sizeof(as_TypeArray(t)->base);
        case TYPE_POINTER:  return 4;
        case TYPE_FUNCTION: return 4;
        case TYPE_STRUCT:   return ((struct Type_struct*)t)->size;
        case TYPE_ERROR:    return 0;
    }
}

// --------------------------------------------------------------------------------
//               is_scalar_type
// ---------------------------------------------------------------------------------
// determine if a type is scalar or aggregate

int is_scalar_type(Type t) {
    switch(t->kind) {
        case TYPE_VOID:     return 1;
        case TYPE_NULL:     return 1;
        case TYPE_BOOL:     return 1;
        case TYPE_CHAR:     return 1;
        case TYPE_SHORT:    return 1;
        case TYPE_INT:      return 1;
        case TYPE_LONG:     return 1;
        case TYPE_REAL:     return 1;
        case TYPE_STRING:   return 1;
        case TYPE_ARRAY:    return 0;
        case TYPE_POINTER:  return 1;
        case TYPE_FUNCTION: return 1;
        case TYPE_STRUCT:   return 0;
        case TYPE_ERROR:    return 1;
    }
}