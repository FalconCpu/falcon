#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "f32.h"

static int hash_size = 1024;
static int hash_count = 0;
static Token* hash_table = 0;

#define MAX_LINE_LENGTH 256
int line_number;
static int line_position;
char current_line[MAX_LINE_LENGTH];

static Token* line_buffer;
static int line_buffer_size = 0;
static int line_buffer_count = 0;

static char* string_buffer;
static int string_buffer_size = 0;
static int string_buffer_count = 0;

static FILE* filehandle;
static int lookahead;

struct Token* all_labels;

string current_block = "";

// ================================================
//                    predefined tokens
// ================================================

static struct Token predefined_tokens[] = {
    { 'i', 0, "0",  0 , 0},
    { '$', 0, "$1", 1 , 0},
    { '$', 0, "$2", 2 , 0},
    { '$', 0, "$3", 3 , 0},
    { '$', 0, "$4", 4 , 0},
    { '$', 0, "$5", 5 , 0},
    { '$', 0, "$6", 6 , 0},
    { '$', 0, "$7", 7 , 0},
    { '$', 0, "$8", 8 , 0},
    { '$', 0, "$9", 9 , 0},
    { '$', 0, "$10", 10 , 0},
    { '$', 0, "$11", 11 , 0},
    { '$', 0, "$12", 12 , 0},
    { '$', 0, "$13", 13 , 0},
    { '$', 0, "$14", 14 , 0},
    { '$', 0, "$15", 15 , 0},
    { '$', 0, "$16", 16 , 0},
    { '$', 0, "$17", 17 , 0},
    { '$', 0, "$18", 18 , 0},
    { '$', 0, "$19", 19 , 0},
    { '$', 0, "$20", 20 , 0},
    { '$', 0, "$21", 21 , 0},
    { '$', 0, "$22", 22 , 0},
    { '$', 0, "$23", 23 , 0},
    { '$', 0, "$24", 24 , 0},
    { '$', 0, "$25", 25 , 0},
    { '$', 0, "$26", 26 , 0},
    { '$', 0, "$27", 27 , 0},
    { '$', 0, "$28", 28 , 0},
    { '$', 0, "$29", 29 , 0},
    { '$', 0, "$30", 30 , 0},
    { '$', 0, "$sp", 31 , 0},

    { 'D', 0, "ld",  0 , 0},

    { 'A', 0, "and",  0 , 0},
    { 'A', 0, "or",   1 , 0},
    { 'A', 0, "xor",  2 , 0},
    { 'A', 0, "add",  4 , 0},
    { 'A', 0, "sub",  5 , 0},
    { 'A', 0, "clt",  6 , 0},
    { 'A', 0, "cltu", 7 , 0},

    { 'H', 0, "lsl",  0 , 0},
    { 'H', 0, "lsr", -2 , 0},
    { 'H', 0, "asr", -1 , 0},

    { 'L', 0, "ldb", 0 , 0},
    { 'L', 0, "ldh", 1 , 0},
    { 'L', 0, "ldw", 2 , 0},

    { 'M', 0, "mul",  0 , 0},
    { 'M', 0, "divu", 4 , 0},
    { 'M', 0, "divs", 5 , 0},
    { 'M', 0, "modu", 6 , 0},
    { 'M', 0, "mods", 7 , 0},

    { 'J', 0, "jmp",  0 , 0},
    { 'B', 0, "beq",  0 , 0},
    { 'B', 0, "bne",  1 , 0},
    { 'B', 0, "blt",  2 , 0},
    { 'B', 0, "bge",  3 , 0},
    { 'B', 0, "bltu", 4 , 0},
    { 'B', 0, "bgeu", 5 , 0},
    { 'j', 0, "jsr",  0 , 0},
    { 'k', 0, "ret",  0 , 0},

    { 'S', 0, "stb", 0 , 0},
    { 'S', 0, "sth", 1 , 0},
    { 'S', 0, "stw", 2 , 0},

    { 'd', 0, "dcb",  0 , 0},
    { 'd', 0, "dch",  1 , 0},
    { 'd', 0, "dcw",  2 , 0},
    { 'z', 0, "ds", 0 , 0},

    { ':', 0, ":", 0 , 0},
    { ',', 0, ",", 0 , 0},
    { '[', 0, "[", 0 , 0},
    { ']', 0, "]", 0 , 0},
    { '=', 0, "=", 0 , 0},
    { '?', 0, "<error>", 0 , 0},

    {0,0,0,0,0}
};

// ================================================
//                    hash table
// ================================================
// Build a hash table of all the tokens seen so far.

static int hash_function(string text) {
    int hash = 0;
    for (int i = 0; text[i]; i++) {
        hash = (hash * 97 + text[i]) % hash_size;
    }
    return hash;
}

static Token hash_find(string text) {
    int hash = hash_function(text);
    while (hash_table[hash]) {
        if (!strcmp(hash_table[hash]->text, text))
            return hash_table[hash];
        hash = (hash + 1) % hash_size;
    }
    return 0;
}

static Token hash_add(Token token) {
    if (hash_count*4 > hash_size * 3) {
        // Hash table is >75% full - resize it to be double the size
        int old_size = hash_size;
        Token* old = hash_table;
        
        hash_size *= 2;
        hash_count = 0;
        hash_table = calloc(hash_size, sizeof(Token));
        if (hash_table==0)
            fatal("out of memory allocating hash table");
        for(int k=0; k<old_size; k++)
            if (old[k])
                hash_add(old[k]);
        free(old);
    }

    int hash = hash_function(token->text);
    while (hash_table[hash])
        hash = (hash + 1) % hash_size;
    hash_table[hash] = token;
    hash_count++;
    return token;
}

// ================================================
//                    iniitialize hash table
// ================================================

static void initialize_hash_table(void) {
    hash_table = calloc(hash_size, sizeof(Token));
    if (hash_table==0)
        fatal("out of memory allocating hash table");

    for (int k=0; predefined_tokens[k].text; k++)
        hash_add(&predefined_tokens[k]);

    line_buffer_size = 64;
    line_buffer = calloc(line_buffer_size, sizeof(Token));

    string_buffer_size = 64;    
    string_buffer = calloc(string_buffer_size, sizeof(char));
}

// ================================================
//                    open_file
// ================================================

void open_file(string filename) {
    if (hash_table==0)
        initialize_hash_table();
    fopen_s(&filehandle, filename, "r");
    if (filehandle==0)
        fatal("could not open file '%s'", filename);
}

// ================================================
//                    line_buffer
// ================================================

static void add_to_line_buffer(Token token) {
    if (line_buffer_count+1 == line_buffer_size) {
        line_buffer_size = line_buffer_size * 2;
        line_buffer = realloc(line_buffer, line_buffer_size * sizeof(Token));
        if (line_buffer==0)
            fatal("out of memory allocating line buffer");
    }
    line_buffer[line_buffer_count++] = token;
    line_buffer[line_buffer_count] = 0;
}

static void clear_line_buffer() {
    line_buffer_count = 0;
    line_buffer[0] = 0;
}

// ================================================
//                    string_buffer
// ================================================

static void add_to_string_buffer(char c) {
    if (string_buffer_count+1 == string_buffer_size) {
        string_buffer_size = string_buffer_size * 2;
        string_buffer = realloc(string_buffer, string_buffer_size);
        if (string_buffer==0)
            fatal("out of memory allocating string buffer");
    }
    string_buffer[string_buffer_count++] = c;
    string_buffer[string_buffer_count] = 0;
}

static void clear_string_buffer() {
    string_buffer_count = 0;
    string_buffer[0] = 0;
}

// ================================================
//                    next_char
// ================================================

static int next_char() {
    int ret = lookahead;

    line_position++;
    lookahead = current_line[line_position];
    if (lookahead == '\n')
        lookahead = 0;

    return ret;
}

// ================================================
//                    read_word
// ================================================

static string read_word() {
    clear_string_buffer();
    do {
        add_to_string_buffer(next_char());
    } while (isalnum(lookahead) || lookahead=='_' || lookahead=='/');

    return string_buffer;
}

static string read_char() {
    clear_string_buffer();
    add_to_string_buffer(next_char());
    return string_buffer;
}

static string append_word(string base) {
    clear_string_buffer();
    for(string p = base; *p; p++)
        add_to_string_buffer(*p);

    do {
        add_to_string_buffer(next_char());
    } while (isalnum(lookahead) || lookahead=='_');

    return string_buffer;
}

// ================================================
//                    next_escaped_char
// ================================================

static char next_escaped_char() {
    char ret = next_char();
    if (ret=='\\') {
        ret = next_char();
        if (ret=='n')
            ret = '\n';
        else if (ret=='t')
            ret = '\t';
        else if (ret=='r')
            ret = '\r';
        else if (ret=='\\')
            ret = '\\';
        else if (ret=='\0')
            ret = '\0';
    }
    return ret;
}


// ================================================
//                    read_string
// ================================================

static string read_string() {
    clear_string_buffer();
    add_to_string_buffer(next_char());
    while (lookahead!='"' && lookahead)
        add_to_string_buffer(next_escaped_char());
    if (lookahead!='"') 
        error("unterminated string");
    add_to_string_buffer(next_char());
    return string_buffer;
}



// ================================================
//                    read_char_lit
// ================================================

static string read_char_lit() {
    clear_string_buffer();
    add_to_string_buffer(next_char());
    while (lookahead!='\'' && lookahead)
        add_to_string_buffer(next_escaped_char());
    if (lookahead!='\'') 
        error("unterminated string");
    add_to_string_buffer(next_char());
    return string_buffer;
}

// ================================================
//           skip_whitespace_and_comments
// ================================================

static void skip_whitespace_and_comments() {
    while (lookahead==' ' || lookahead=='\t' || lookahead=='\n')
        next_char();
    if (lookahead=='#')
        while(lookahead)
            next_char();
}

// ================================================
//           new_token
// ================================================

static Token new_token(int kind, string text, int value) {
    Token token = my_malloc(sizeof(struct Token));
    token->kind = kind;
    token->text = _strdup(text);
    token->value = value;
    return hash_add(token);
}

// ================================================
//             my_atoi
// ================================================

static int my_atoi(string s) {
    int sign = 1;
    if (s[0]=='-') {
        sign = -1;
        s++;
    }

    if (s[0]=='0' && s[1]=='x') {
        int ret = 0;
        for (int k=2; s[k]; k++) {
            ret = ret * 16;
            if (isdigit(s[k]))
                ret += s[k] - '0';
            else if (s[k]>='a' && s[k]<='f')
                ret += s[k] - 'a' + 10;
            else if (s[k]>='A' && s[k]<='F')
                ret += s[k] - 'A' + 10;
            else
                fatal("bad hex number '%s'", s);
        }
        return ret * sign;
    } else {
        int ret = 0;
        for (int k=0; s[k]; k++)
            ret = ret * 10 + s[k] - '0';
        return ret * sign;
    }
}




// ================================================
//           read_token
// ================================================

static Token read_token() {
    skip_whitespace_and_comments();
    Token ret = 0;

    if (lookahead==0)
        return 0;

    else if (lookahead=='$') {
        string text = read_word();
        ret = hash_find(text);
        if (ret==0) {
            error("unknown register '%s'", text);
            ret = hash_find("$1");
        }
    
    } else if (isalpha(lookahead) || lookahead=='_' || lookahead=='/') {
        string text = read_word();
        ret = hash_find(text);
        if (ret==0) {
            ret = new_token('l', text, 0);
            ret->next = all_labels;
            all_labels = ret;
        }
        
    } else if (lookahead=='.' || lookahead=='@') {
        string text = append_word(current_block);
        ret = hash_find(text);
        if (ret==0) {
            ret = new_token('l', text, 0);
            ret->next = all_labels;
            all_labels = ret;
        }

    } else if (isdigit(lookahead) || lookahead=='-') {
            string text = read_word();
            ret = hash_find(text);
            if (ret==0) 
                ret = new_token('i', text, my_atoi(text));

    } else if (lookahead=='"') {
        string text = read_string();
        ret = hash_find(text);
        if (ret==0)
            ret = new_token('"', text, 0);

    } else if (lookahead=='\'') {
        string text = read_char_lit();
        ret = hash_find(text);
        if (ret==0) {
            if (strlen(text)!=3)
                error("Invalid char literal '%s'", text);
            ret = new_token('i', text, text[1]);
        }

    } else {    
        string text = read_char();
        ret = hash_find(text);
        if (ret==0) {
            error("unknown character '%s'", text);
            ret = hash_find("<error>");
        }
    }

    return ret;
}

// ================================================
//                    read_line
// ================================================

Token* read_line() {
    line_number++;
    clear_line_buffer();

    string s = fgets(current_line, MAX_LINE_LENGTH, filehandle);
    if (s==0)
        return 0;

    line_position = 0;
    lookahead = current_line[0];

    while (lookahead) {
        Token token = read_token();
        add_to_line_buffer(token);
    }

    return line_buffer;
}
