#include "fpl.h"
#include "types.h"
#include "instr.h"

struct Stdlib stdlib;

void initialize_stdlib() {
    // malloc - allocate a block of memory
    TypeList params = new_TypeList();
    TypeList_add(params, type_int);
    Type func_type = make_type_function(params, type_voidstar);
    stdlib.sdlib_malloc = new_Symbol(0, SYM_FUNCTION, "malloc", func_type, 0);

    // bzero(address:void*, size:Int)
    // zeros out a block of memory
    params = new_TypeList();
    TypeList_add(params, type_voidstar);
    TypeList_add(params, type_int);
    func_type = make_type_function(params, type_void);
    stdlib.sdlib_bzero = new_Symbol(0, SYM_FUNCTION, "bzero", func_type, 0);

    // memcpy(dest:void*, source:void*, size:Int)
    // zeros out a block of memory
    params = new_TypeList();
    TypeList_add(params, type_voidstar);
    TypeList_add(params, type_voidstar);
    TypeList_add(params, type_int);
    func_type = make_type_function(params, type_void);
    stdlib.sdlib_memcpy = new_Symbol(0, SYM_FUNCTION, "memcpy", func_type, 0);

    // new(type_desctiptor)
    params = new_TypeList();
    TypeList_add(params, type_voidstar);
    func_type = make_type_function(params, type_voidstar);
    stdlib.stdlib_new = new_Symbol(0, SYM_FUNCTION, "_new", func_type, 0);

    // delete(voidstar)
    params = new_TypeList();
    TypeList_add(params, type_voidstar);
    func_type = make_type_function(params, type_void);
    stdlib.stdlib_delete = new_Symbol(0, SYM_FUNCTION, "_delete", func_type, 0);

    // size fields for strings/arrays
    stdlib.size = new_Symbol(0, SYM_VARIABLE, "size", type_int, 0);
    stdlib.size->offset = -4;
}


// ---------------------------------------------------------------------------------
//                           gen_memcopy
// ---------------------------------------------------------------------------------
// generate code to copy a block of memory

void gen_memcopy(Function func, Symbol dest, Symbol source, int size) {
    add_instr(func, new_Instr(INSTR_MOV, 0, SymbolList_get(func->all_vars,1), dest, 0));
    add_instr(func, new_Instr(INSTR_MOV, 0, SymbolList_get(func->all_vars,2), source, 0));
    add_instr(func, new_Instr(INSTR_MOV, 0, SymbolList_get(func->all_vars,3), make_constant_symbol(func, size), 0));
    add_instr(func, new_Instr(INSTR_CALL, 0, 0, stdlib.sdlib_memcpy, 0));
}