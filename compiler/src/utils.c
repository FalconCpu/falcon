#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "fpl.h"

int num_errors;

// ==========================================
//                fatal
// ==========================================
// Prints an error message and exits the program.

void  fatal(String message, ...) {
    va_list va;
    va_start(va, message);
    vprintf(message, va);
    printf("\n");
    va_end(va);
    exit(1);
}

// ==========================================
//                error
// ==========================================
// Prints an error message

void  error(Location* location, String message, ...) {
    va_list va;
    va_start(va, message);
    if (location!=0)
        printf("%s:%d.%d: ", location->filename, location->line, location->column);
    vprintf(message, va);
    printf("\n");
    va_end(va);
    num_errors++;
}

// ==========================================
//                my_malloc
// ==========================================
// Allocates memory. Aborts the program if the allocation fails.

void* my_malloc(int size) {
    void *ret = calloc(size,1);
    if (ret==0)
        fatal("Out of memory");
    return ret;
}

// ==========================================
//                my_realloc
// ==========================================
// Allocates memory. Aborts the program if the allocation fails.

void* my_realloc(void* ptr, int size) {
    void *ret = realloc(ptr, size);
    if (ret==0)
        fatal("Out of memory");
    return ret;
}


