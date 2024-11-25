#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "f32.h"

int line_number;
int* program;
int program_size;

static void load_program(string filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL)
        fatal("Can't open file '%s'", filename);

    program = my_malloc(65536);
    program_size = 0;

    char line[100];
    while (fgets(line, sizeof(line), file) != NULL)
        program[program_size++] = strtol(line,0,16);
    fclose(file);
}


int main(int argc, char** argv) {
    string filename;

    if (argc < 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    } else {
        filename = argv[1];
    }
    
    load_program(filename);
    load_labels("asm.labels");
    disassemble_program(program, program_size);

    return 0;
}
