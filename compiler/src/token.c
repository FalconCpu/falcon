#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "fpl.h"
#include "location.h"
#include "token.h"

static FILE* fh;
extern Location yylloc;
extern String yylval;
static int lookahead;
static int at_start_of_line;
static int line_continues;

static String file_name;
static int line_number;
static int column_number;
static int first_line;
static int first_column;
static int last_line;
static int last_column;

#define MAX_INDENT 64
static int indent_stack[MAX_INDENT];
static int indent_level;

// ======================================================================
//                             new_token
// ======================================================================

static Token new_token(TokenKind kind, String text) {
    Token ret = new(struct Token);
    ret->location = new_location(file_name, first_line, first_column, last_line, last_column);
    ret->kind = kind;
    ret->text = text;
    return ret;
}

// ======================================================================
//                             hash_table
// ======================================================================
// keep a hash table containing all the different words seen so far
// mapping them to a 'kind' value (listed in fpl.h).
// 


typedef struct {
    String     name;
    TokenKind  kind;
} Hash_Element;

static Hash_Element *hash_table;
static int hash_size;
static int hash_count;


static int hash_function(String txt) {
    int ret = 0;
    for(; *txt; txt++)
        ret = (ret * 31) + *txt;
    return ret & (hash_size-1);
}

static Hash_Element* hash_add(String name, TokenKind kind) {
    if (hash_count*4 > hash_size*3) {
        Hash_Element *old_table = hash_table;
        hash_size *= 2;
        hash_table = new_array(Hash_Element, hash_size);
        hash_count = 0;
        for(int k=0; k<hash_size/2; k++)
            if (old_table[k].name)
                hash_add(old_table[k].name, old_table[k].kind);
        free(old_table);
    }

    int slot = hash_function(name);
    while(hash_table[slot].name)
        slot = (slot+1) & (hash_size-1);
    hash_table[slot].name = name;
    hash_table[slot].kind = kind;
    return &hash_table[slot];
}

static Hash_Element* hash_find(String name) {
    int slot = hash_function(name);
    while(hash_table[slot].name)
        if (!strcmp(hash_table[slot].name, name))
            return &hash_table[slot];
        else
            slot = (slot+1) & (hash_size-1);
    return 0;
}

// ======================================================================
//                             predefined tokens
// ======================================================================

String token_kind_names[] = {
    [TOK_EOF       ] = "<eof>",
    [TOK_EOL       ] = "<eol>",
    [TOK_INDENT    ] = "<indent>",
    [TOK_DEDENT    ] = "<dedent>",
    [TOK_ID        ] = "<id>",
    [TOK_INTLIT    ] = "<intlit>",
    [TOK_CHARLIT   ] = "<charlit>",
    [TOK_STRLIT    ] = "<strlit>",
    [TOK_REALLIT   ] = "<reallit>",
    [TOK_PLUS      ] = "+",
    [TOK_MINUS     ] = "-",
    [TOK_STAR      ] = "*",
    [TOK_SLASH     ] = "/",
    [TOK_PERCENT   ] = "%",
    [TOK_AMPERSAND ] = "&",
    [TOK_BAR       ] = "|",
    [TOK_CARAT     ] = "^",
    [TOK_EQ        ] = "=",
    [TOK_NEQ       ] = "!=",
    [TOK_LT        ] = "<",
    [TOK_LTE       ] = "<=",
    [TOK_GT        ] = ">",
    [TOK_GTE       ] = ">=",
    [TOK_AND       ] = "and",
    [TOK_OR        ] = "or",
    [TOK_DOT       ] = ".",
    [TOK_COMMA     ] = ",",
    [TOK_COLON     ] = ":",
    [TOK_LEFT      ] = "<<",
    [TOK_RIGHT     ] = ">>",
    [TOK_ARROW     ] = "->",
    [TOK_OPENB     ] = "(",
    [TOK_OPENSQ    ] = "[",
    [TOK_OPENCL    ] = "{",
    [TOK_CLOSEB    ] = ")",
    [TOK_CLOSESQ   ] = "]",
    [TOK_CLOSECL   ] = "}",
    [TOK_VAL       ] = "val",
    [TOK_VAR       ] = "var",
    [TOK_IF        ] = "if",
    [TOK_END       ] = "end",
    [TOK_ELSE      ] = "else",
    [TOK_REPEAT    ] = "repeat",
    [TOK_UNTIL     ] = "until",
    [TOK_WHILE     ] = "while",
    [TOK_RETURN    ] = "return",
    [TOK_FUN       ] = "fun",
    [TOK_STRUCT    ] = "struct",
    [TOK_QMARK     ] = "?",
    [TOK_EMARK     ] = "!",
    [TOK_QMARKDOT  ] = "?.",
    [TOK_QMARKCOLON] = "?:",
    [TOK_NEW]        = "new",
    [TOK_IN]         = "in",
    [TOK_TO]         = "to",
    [TOK_FOR]        = "for",
    [TOK_EXTERN]     = "extern",
    [TOK_NOT]        = "not",
    [TOK_ELSIF]      = "elsif",
    [TOK_ERROR     ] = "error",
    0
};

// ======================================================================
//                            Initialize_hash_table
// ======================================================================
// Populate the hash table with all the keywords and punctuation values

static void initialize_hash_table() {
    if (hash_table!=0)
        return; // already been initialized

    hash_size = 256; // initial size 
    hash_count = 0;
    hash_table = new_array(Hash_Element, hash_size);

    for(TokenKind k=0; token_kind_names[k]; k++)
        hash_add(token_kind_names[k], k);
    initialize_types();
}


// ======================================================================
//                             string buffer
// ======================================================================
// dynamically allocated string buffer - just used to temporarily hold
// the contents of a string as we are reading it.

static char* string_buffer;
static int   string_length;
static int   string_alloc;

static String add_to_string(int c) {
    if (string_length+1==string_alloc) {
        string_alloc *= 2;
        resize_array(string_buffer, char, string_alloc);
    }

    string_buffer[string_length++] = c;
    string_buffer[string_length] = 0;
    return string_buffer;
}

static void clear_string() {
    if (string_alloc==0) {
        string_alloc = 64;
        string_buffer = new_array(char, string_alloc);
    }
    string_length = 0;
    string_buffer[0] = 0;
}

// ======================================================================
//                             open file
// ======================================================================
// Open a file and prepare to read from it
// Exit the program if the file cannot be opened

void open_file(String fileName) {
    initialize_hash_table();
    //initialize_types();

    fh = fopen(fileName,"r");
    if (fh==0) {
        fatal("Cannot open file '%s'", file_name);
        exit(10);
    }
    
    indent_level = 0;
    indent_stack[indent_level]=1;

    file_name      = fileName;
    line_number    = 1;
    column_number  = 1;

    lookahead = fgetc(fh);
    at_start_of_line = 1;
    line_continues = 1;
}



// ======================================================================
//                             next char
// ======================================================================
// read the next character from the current file, and updates location

static int next_char() {
    int ret = lookahead;
    last_line = line_number;
    last_column = column_number;
    lookahead = fgetc(fh);

    if (ret=='\n') {
        line_number ++;
        column_number = 1;
    } else
        column_number++;

    return ret;
}


// ======================================================================
//                             next char escape
// ======================================================================
// reads a char - and handles excapes

static int next_char_escape() {
    int ret = next_char();
    if (ret=='\\') {
        ret = next_char();
        switch(ret) {
            case 'n' : ret = '\n'; break;
            case '0' : ret = '\0'; break;
            case 't' : ret = '\t'; break;
            // any other character after a \ gets passed through
        }
    }
    return ret;
}

// ======================================================================
//                             read string
// ======================================================================
// read a string - enclosed in "

static String read_string() {
    add_to_string(next_char()); // get the opening "
    while(lookahead!='"' && lookahead!=EOF)
        add_to_string(next_char_escape());
    if (lookahead=='"') 
        add_to_string(next_char()); // add the closing "
    else
        error(new_location(file_name, first_line, first_column, last_line,last_column), "Unterminated string");
    return string_buffer;
}


// ======================================================================
//                             read_word
// ======================================================================
// reads a sequence of alphanumeric characters and returns as a string

static String read_word() {
    do {
        add_to_string(next_char());
    } while(isalnum(lookahead) || lookahead=='_');
    return string_buffer;
}

// ======================================================================
//                             read_punctuation
// ======================================================================
// reads a sequence of non alphanumeric characters that form a single token

static String read_punctuation() {
    clear_string();
    int c = next_char();
    add_to_string(c);
    if ( (c=='<' && lookahead=='=') ||
         (c=='>' && lookahead=='=') ||
         (c=='!' && lookahead=='=') ||
         (c=='<' && lookahead=='<') ||
         (c=='>' && lookahead=='>') ||
         (c=='-' && lookahead=='>') ||
         (c=='?' && lookahead=='.') ||
         (c=='?' && lookahead==':'))
         add_to_string(next_char());
    return string_buffer;
}


// ======================================================================
//                             skip whitespace and comments
// ======================================================================
// skip whitespace and comments
// newlines are skipped if the previous token was a binary operator

static void skip_whitespace_and_comments() {
    while (lookahead==' ' || lookahead=='\t' || lookahead=='#' || lookahead==';' || (lookahead=='\n' && line_continues))
        if (lookahead==';' || lookahead=='#')
            while(lookahead!='\n' && lookahead!=EOF)
                next_char();
        else
            next_char();
}

// ======================================================================
//                             read_token
// ======================================================================
// Read the next token from the file

Token read_token() {
    skip_whitespace_and_comments();
    first_line = line_number;
    first_column = column_number;

    Hash_Element* he = 0;
    
    while(he==0) {
        clear_string();
        if (lookahead==EOF || (at_start_of_line && lookahead=='@')) {
            if (!at_start_of_line) 
                he = hash_find("<eol>");
            else if (indent_level>0) {
                he = hash_find("<dedent>");
                indent_level--;
            } else
                he = hash_find("<eof>");
        
        } else if (lookahead=='\n') {
            next_char();
            he = hash_find("<eol>");

        } else if (at_start_of_line && column_number > indent_stack[indent_level]) {
            he = hash_find("<indent>");
            if (indent_level==MAX_INDENT)
                fatal("Max of %d levels of indentation", MAX_INDENT);
            indent_level++;
            indent_stack[indent_level] = column_number;

        } else if (at_start_of_line && column_number < indent_stack[indent_level]) {
            he = hash_find("<dedent>");
            if (indent_level==0)
                error(new_location(file_name, first_line, first_column, last_line,last_column), "Indentation error, more dedents than indents");
            else
                indent_level--;
            if (column_number > indent_stack[indent_level]) {
                error(new_location(file_name, first_line, first_column, last_line,last_column), "Indentation error - expected indentation to column %d", indent_stack[indent_level]);
                indent_stack[indent_level] = column_number;
            }


        } else if (isalpha(lookahead) || lookahead=='_') {
            String s = read_word();
            he = hash_find(s);
            if (he==0)
                he = hash_add(strdup(s), TOK_ID);

        } else if (isdigit(lookahead)) {
            String s = read_word();
            if (lookahead=='.') {
                // Got a Real literal
                add_to_string(next_char());  // add the '.' to the string buffer
                if (isdigit(lookahead))
                    read_word();  // append the rest of the number into the string buffer
                he = hash_find(s);
                if (he==0)
                    he = hash_add(strdup(s), TOK_REALLIT);
            } else {
                // Got an integer literal
                he = hash_find(s);
                if (he==0)
                    he = hash_add(strdup(s), TOK_INTLIT);
            }

        } else if (lookahead=='.') {
            String s = add_to_string(next_char());  // put the '.' into the string buffer
            if (isdigit(lookahead))
                read_word();  // append any following alphanumerics into the buffer
            // s will now either contain a single '.' or a '.' followed by alphanumerics
            // The hash table will already contain an entry for '.' mapping to TOK_DOT
            // anything else we will map to TOK_REALLIT. 
            // We deffer detecting malformed floats to the type checking phase.
            he = hash_find(s);
            if (he==0)
                he = hash_add(strdup(s), TOK_REALLIT);

            
        } else if (lookahead=='\"') {
            String s = read_string();
            he = hash_find(s);
            if (he==0)
                he = hash_add(strdup(s), TOK_STRLIT);

        } else if (lookahead=='\'') {
            String s = add_to_string(next_char()); // get the opening '
            add_to_string(next_char_escape());
            if (lookahead=='\'')
                add_to_string(next_char()); // get the closing '
            else
                error(new_location(file_name, first_line, first_column, last_line,last_column), "Unterminated char literal");
            he = hash_find(s);
            if (he==0)
                he = hash_add(strdup(s), TOK_CHARLIT);

        } else {
            String s = read_punctuation();
            he = hash_find(s);
            if (he==0)
                error(new_location(file_name, first_line, first_column, last_line,last_column), "Unrecognized token '%s'", s);
        }
    }

    line_continues = (he->kind>=TOK_PLUS && he->kind<=TOK_OPENCL) || he->kind==TOK_EOL;
    at_start_of_line = he->kind==TOK_EOL || he->kind==TOK_DEDENT;

    return new_token(he->kind, he->name);
}


// ======================================================================
//                             unique_string
// ======================================================================
// Create a unique pointer for any string value. This allows strings to be compared
// by pointer equality, rather than requiring strcmp
String unique_string(String s) {
    Hash_Element* he = hash_find(s);
    if (he==0) {
        he = hash_add(strdup(s), TOK_ID);
    }
    return he->name;
}






