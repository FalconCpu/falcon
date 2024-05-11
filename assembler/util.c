#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "f32asm.h"

int num_errors;
extern int line_number;
extern string file_name;

/// -----------------------------------------------------
///                       fatal
/// -----------------------------------------------------
void fatal(string message, ...) {
    va_list va;
    va_start(va, message);
    printf("FATAL: ");
    vprintf(message, va);
    printf("\n");
    exit(20);
}

/// -----------------------------------------------------
///                       error
/// -----------------------------------------------------
void error(string message,...) {
    va_list va;
    va_start(va, message);
    printf("%s line %d:",file_name, line_number);
    vprintf(message, va);
    printf("\n");
    num_errors++;
}

/// -----------------------------------------------------
///                       my_malloc
/// -----------------------------------------------------
void* my_malloc(int size) {
    void *ret = calloc(1,size);
    if (ret==0)
        fatal("Out of memory");
    return ret;
}

/// -----------------------------------------------------
///                       my_realloc
/// -----------------------------------------------------
void* my_realloc(void *old, int new_size) {
    void *ret = realloc(old,new_size);
    if (ret==0)
        fatal("Out of memory");
    return ret;
}


