#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "f32.h"

extern int num_errors;

int fileFormat = 0;

int main(int argc, char** argv) {
    string output_filename = "asm.hex";

    if (argc < 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    initialize_assembler();

    for(int i=1; i<argc; i++) {
        if (!strcmp(argv[i], "-bin")) {
            fileFormat = FILE_FORMAT_BIN;
    
        } else if (!strcmp(argv[i], "-hex")) {
            fileFormat = FILE_FORMAT_HEX;
        
        } else if (!strcmp(argv[i], "-hunk")) {
            fileFormat = FILE_FORMAT_HUNK;
    
        } else if (!strcmp(argv[i], "-o")) {
            if (i+1 < argc) {
                output_filename = argv[++i];
            } else {
                printf("Usage: %s <filename> -o <output_filename>\n", argv[0]);
                return 1;
            }
    
        } else
            assemble_file(argv[i]);
    }
    
    output_result(output_filename, fileFormat);

    return num_errors;
}
