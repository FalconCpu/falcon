#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "f32.h"

int line_number;
int* prog_mem;
unsigned int* data_mem;

FILE* trace_file  = NULL;

static void load_program(string filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL)
        fatal("Can't open file '%s'", filename);

    char line[100];
    int program_size = 0;
    while (fgets(line, sizeof(line), file) != NULL)
        prog_mem[program_size++] = strtoul(line,0,16);
    fclose(file);
}


int main(int argc, char** argv) {
    prog_mem = my_malloc(65536);
    data_mem = my_malloc(64*1024*1024);

    string filename;

    if (argc==2) {
        filename = argv[1];
    } else if (argc==3 && strcmp(argv[1], "-t")==0) {
        trace_file = fopen("sim_traace.log", "w");
        filename = argv[2];
    } else {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    load_program(filename);
    load_labels("asm.labels");
    execute();

    return 0;
}
