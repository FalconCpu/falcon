#include <stdio.h>
#include <string.h>

#include "fpl.h"
#include "token.h"
#include "location.h"

enum Operations {
    OPERATION_LEX_ONLY,
    OPERATION_PARSE_ONLY,
    OPERATION_TYPE_CHECK,
    OPERATION_CODE_GEN,
    OPERATION_PEEPHOLE,
    OPERATION_REGALLOC,
    OPERATION_FULL_COMPILE
};

static enum Operations operation = OPERATION_FULL_COMPILE;

extern int num_errors;
Block top_block;
int debug = 0;

// ======================================================================
//                             parse_option
// ======================================================================

static void parse_option(String s) {
    if (!strcmp(s,"--lex"))
        operation = OPERATION_LEX_ONLY;
    else if (!strcmp(s,"--parse"))
        operation = OPERATION_PARSE_ONLY;
    else if (!strcmp(s,"--typecheck"))
        operation = OPERATION_TYPE_CHECK;
    else if (!strcmp(s,"--codegen"))
        operation = OPERATION_CODE_GEN;
    else if (!strcmp(s,"--peephole"))
        operation = OPERATION_PEEPHOLE;
    else if (!strcmp(s,"--regalloc"))
        operation = OPERATION_REGALLOC;
    else if (!strcmp(s,"--debug"))
        debug = 1;
    else
        fatal("Undefined command line switch '%s'",s);
}

// ======================================================================
//                             process_file
// ======================================================================

static void process_file(String file_name) {
    open_file(file_name);

    if (operation==OPERATION_LEX_ONLY) {
        Token t;
        do {
            t = read_token();
            printf("%d.%d-%d.%d %s %s\n", t->location->first_line, t->location->first_column, t->location->last_line, 
                        t->location->last_column, token_kind_names[t->kind], t->text);
        } while(t->kind!=TOK_EOF);

    } else {
        parse_top(top_block);
    }
}



// ======================================================================
//                             main
// ======================================================================

int main(int argc, String* argv) {
    top_block = new_block(0);

    // Process the individual input files
    for(int index=1; index<argc; index++) {
        if (argv[index][0]=='-')
            parse_option(argv[index]);
        else
            process_file(argv[index]);
    }


    if (num_errors!=0)
        return num_errors;
    if (operation==OPERATION_LEX_ONLY)
        return 0;
    if (operation==OPERATION_PARSE_ONLY) {
        block_print(top_block, 0);
        return num_errors;
    }

    //
    // Run type checking
    //
    typecheck(top_block);
    if (num_errors!=0)
        return num_errors;
    if (operation==OPERATION_TYPE_CHECK) {
        block_print(top_block, 0);
        return 0;
    }

    printf("XXXX\n");

}
