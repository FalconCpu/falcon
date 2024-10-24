#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "fpl.h"

extern int num_errors;

#define STOP_AT_PARSE     1
#define STOP_AT_TYPECHECK 2
static int stop_at = 0;

// ==========================================
//                compile_file
// ==========================================
// Compiles a file.

extern Block top_block;

void compile_file(String filename) {
    top_block = new_block();

    open_file(filename);
    yyparse();
}

// ==========================================
//                run_type_check
// ==========================================

static void run_type_check() {
    
    build_hierarchy_block(top_block, 0);
    type_check_block(top_block);
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
            printf("  -parse_only\n");
            printf("  -typecheck_only\n");
            exit(0);
        } else if (!strcmp(argv[i], "-parse_only"))
            stop_at = STOP_AT_PARSE;
        else if (!strcmp(argv[i], "-typecheck_only"))
            stop_at = STOP_AT_TYPECHECK;
        else
            compile_file(argv[i]);
    }
}


// ==========================================
//                main
// ==========================================
// The main function of the compiler.

int main(int argc, char *argv[]) {
    parse_args(argc, argv);

    if (num_errors)
        return 1;
    if (stop_at==STOP_AT_PARSE) {
        print_block(top_block, 0);
        return 0;
    }

    run_type_check();
    if (num_errors)
        return 1;
    if (stop_at==STOP_AT_TYPECHECK) {
        print_block(top_block, 0);
        return 0;
    }

    return 0;
}
