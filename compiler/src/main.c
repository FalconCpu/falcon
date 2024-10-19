#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "fpl.h"
#include "ast.h"

CompilerStopAt stop_at = STOP_AT_NONE;

// ==========================================
//                compile_file
// ==========================================
// Compiles a file.

extern AST ast_top;

void compile_file(String filename) {
    open_file(filename);
    yyparse();
    print_ast(ast_top, 0);
}

// ==========================================
//                parse_args
// ==========================================
// Parses the command line arguments.

void parse_args(int argc, char *argv[]) {
    for (int i=1; i<argc; i++) {
        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            printf("Usage: fpl [options] files\n");
            printf("Options:\n");
            printf("  -lex_only\n");
            printf("  -ast_only\n");
            printf("  -ir_only\n");
            printf("  -asm_only\n");
            exit(0);
        } else if (!strcmp(argv[i], "-lex_only")) {
            stop_at = STOP_AT_LEXER;
        } else if (!strcmp(argv[i], "-ast_only")) {
            stop_at = STOP_AT_AST;
        } else if (!strcmp(argv[i], "-ir_only")) {
            stop_at = STOP_AT_IR;
        } else if (!strcmp(argv[i], "-asm_only")) {
            stop_at = STOP_AT_ASM;
        } else {
            compile_file(argv[i]);
        }
    }
}


// ==========================================
//                main
// ==========================================
// The main function of the compiler.

int main(int argc, char *argv[]) {
    parse_args(argc, argv);
    return 0;
}
