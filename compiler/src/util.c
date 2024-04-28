#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "fpl.h"
#include "location.h"


int num_errors;
int num_warnings;
extern Type type_error;

// ========================================================
//                     fatal
// ========================================================

void fatal(String message, ...) {
    va_list va;
    va_start(va, message);
    printf("FATAL: ");
    vprintf(message,va);
    printf("\n");
    fflush(stdout);
    abort();
}

// ========================================================
//                     error
// ========================================================

void error(Location loc, String message, ...) {
    va_list va;
    va_start(va, message);
    if (loc)
        printf("%s %d.%d-%d.%d:- ", loc->file_name, loc->first_line, loc->first_column, loc->last_line, loc->last_column);
    else    
        printf("ERROR:- ");
    vprintf(message,va);
    printf("\n");
    num_errors++;
}

// ========================================================
//                     warn
// ========================================================

void warn(Location loc, String message, ...) {
    va_list va;
    va_start(va, message);
    if (loc)
        printf("%s %d.%d-%d.%d:- ", loc->file_name, loc->first_line, loc->first_column, loc->last_line, loc->last_column);
    else    
        printf("WARNING:- ");
    vprintf(message,va);
    printf("\n");
    num_warnings++;
}


Type make_type_error(Location loc, String message, ...) {
    va_list va;
    va_start(va, message);
    if (loc)
        printf("%s %d.%d-%d.%d:- ", loc->file_name, loc->first_line, loc->first_column, loc->last_line, loc->last_column);
    else    
        printf("ERROR:- ");
    vprintf(message,va);
    printf("\n");
    num_errors++;
    return type_error;
}


// ========================================================
//                     my_malloc
// ========================================================

void* my_malloc(int size) {
    void *ret = calloc(size,1);
    if (ret==0) 
        fatal("Out of memory");
    return ret;
}

// ========================================================
//                     my_realloc
// ========================================================

void* my_realloc(void *old, int new_size) {
    void* ret = realloc(old, new_size);
    if (ret==0) 
        fatal("Out of memory");
    return ret;
}

// ========================================================
//                     my_strcat2
// ========================================================

String my_strcat2(String s1, String s2) {
    int len = strlen(s1) + strlen(s2) +1;
    char* ret = my_malloc(len);
    strcpy(ret, s1);
    strcat(ret, s2);
    return ret;
}

String my_strcat3(String s1, String s2, String s3) {
    int len = strlen(s1) + strlen(s2) + strlen(s3) + 1;
    char* ret = my_malloc(len);
    strcpy(ret, s1);
    strcat(ret, s2);
    strcat(ret, s3);
    return ret;
}

String my_strcat4(String s1, String s2, String s3, String s4) {
    int len = strlen(s1) + strlen(s2) + strlen(s3) + strlen(s4) + 1;
    char* ret = my_malloc(len);
    strcpy(ret, s1);
    strcat(ret, s2);
    strcat(ret, s3);
    strcat(ret, s4);
    return ret;
}

// ========================================================
//                     location_toString
// ========================================================

String location_toString(Location loc) {
    char buf[64];
    sprintf(buf,"%s %d.%d-%d.%d", loc->file_name, loc->first_line, loc->first_column, loc->last_line, loc->last_column);
    return strdup(buf);
}

// ========================================================
//                     print_spaces
// ========================================================

void print_spaces(int indent) {
    for(int k=0; k<indent; k++)
        printf("   ");
}

