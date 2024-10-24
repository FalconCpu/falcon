#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string>
using std::string;

#include "fpl.h"

int num_errors;
int num_warnings;

// ========================================================
//                     fatal
// ========================================================

void fatal(const string message, ...) {
    va_list va;
    va_start(va, message);
    printf("FATAL: ");
    vprintf(message.c_str(),va);
    printf("\n");
    fflush(stdout);
    abort();
}


// ========================================================
//                     fatal
// ========================================================

void error(const Location& location, const string message, ...) {
    va_list va;
    va_start(va, message);
    printf("%s ",location.get_location().c_str());
    vprintf(message.c_str(),va);
    printf("\n");
    fflush(stdout);
    num_errors++;
}
