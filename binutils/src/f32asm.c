#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "f32.h"

extern int num_errors;

int main(int argc, char** argv) {
    string output_filename = "asm.hex";

    if (argc < 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    initialize_assembler();
    for(int i=1; i<argc; i++)
        assemble_file(argv[i]);
    
    output_result(output_filename);

    return num_errors;
}
