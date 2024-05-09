#include <stdio.h>
#include <stdlib.h>
#include "f32asm.h"

#define MEM_SIZE 0x40000
int prog[MEM_SIZE/4];
int prog_length;
token* all_labels;
int line_number;
string file_name;

extern int num_errors;

int main(int argc, string argv[])
{
    if (argc<2)
        fatal("Usage f32dis filename");
    read_file(argv[1]);

   disassemble();
}
