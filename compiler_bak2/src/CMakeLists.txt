cmake_minimum_required(VERSION 3.20)
project(fplcomp)
find_package(BISON)
add_compile_options(-Wall -std=c99)
set(EXECUTABLE_OUTPUT_PATH "C:/Users/simon/falcon/bin")

BISON_TARGET(fplcomp parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.c
              DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/parser.h)

add_executable(fplcomp main.c utils.c lexer.c parser.c AST.c Block.c AST_list.c type.c
               symbol.c
               AST_IntLiteral.c AST_Identifier.c AST_Binop.c AST_Declaration.c AST_TypeName.c
               AST_Parameter.c AST_list.c AST_Function.c)
