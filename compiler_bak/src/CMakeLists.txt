cmake_minimum_required(VERSION 3.10)
project(fpl)

#find_package(BISON)
#BISON_TARGET(MyParser parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.c
#             DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/parser.h)


set(EXECUTABLE_OUTPUT_PATH "C:/Users/simon/falcon/bin")

add_executable(fpl fpl.c location.c token.c util.c
    AST.c AST_intlit.c AST_symbol.c AST_binop.c AST_decl.c AST_index.c AST_member.c
    AST_unary.c AST_funccall.c AST_return.c AST_assign.c AST_while.c AST_repeat.c AST_new.c
    AST_clause.c AST_if.c AST_struct.c AST_strlit.c AST_pointer.c AST_cast.c AST_for.c
    AST_assignOp.c
    AST_list.c block.c parser.c function.c types.c symbol.c typecheck.c instr.c
    stdlib.c bitmap.c peephole.c livemap.c regalloc.c gen_assembly.c
)

target_compile_options(fpl PRIVATE -Wall -Wextra -Wno-microsoft-enum-forward-reference -Wno-deprecated-declarations)


# AST.c  