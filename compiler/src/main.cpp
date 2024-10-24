#include <iostream>
#include "fpl.h"
using std::string_view;

extern Ast_top* ast_top;
extern int num_errors;

int main() {
    open_file("example.fpl");
    yyparse();
    if (num_errors==0)  
        ast_top->tree_print(0);

    return 0;
}


