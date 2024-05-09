#include <stdio.h>
#include <stdlib.h>
#include "f32asm.h"

extern int num_errors;

int main(int argc, string argv[])
{
    if (argc<2)
        fatal("Usage f32asm filename");
    open_file(argv[1]);

    assemble_file(argv[1]);

    if (num_errors==0)
        disassemble();

}
