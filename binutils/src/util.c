#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "f32.h"

extern char current_line[];
extern int line_number;
int num_errors = 0;

// ================================================
//                    fatal
// ================================================

void fatal(string message, ...) {
    va_list args;
    va_start(args, message);
    vprintf(message, args);
    printf("\n");
    va_end(args);
    exit(1);
}

// ================================================
//                    error
// ================================================

void error(string message, ...) {
    va_list args;
    va_start(args, message);
    printf("Line %d: ", line_number);
    vprintf(message, args);
    printf("\n");
    va_end(args);
    num_errors++;
}

// ================================================
//                    my_malloc
// ================================================

void* my_malloc(size_t size) {
    void* ptr = calloc(size,1);
    if (ptr==0)
        fatal("out of memory allocating %d bytes", size);
    return ptr;
}

// ================================================
//                    my_realloc
// ================================================

void* my_realloc(void* ptr, size_t size) {
    void* new_ptr = realloc(ptr, size);
    if (new_ptr==0)
        fatal("out of memory allocating %d bytes", size);
    return new_ptr;
}