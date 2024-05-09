#include <stdio.h>
#include <stdlib.h>
#include "f32asm.h"

extern int num_errors;

int main(int argc, string argv[])
{
    if (argc<2)
        fatal("Usage f32asm filename");

    for(int index=1; index<argc; index++)
        assemble_file(argv[index]);

    if (num_errors==0)
        output_file("rom.hex");
    return num_errors;
}
