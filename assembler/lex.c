#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "f32asm.h"

int line_number;
string file_name;
token* all_labels=0;
token* all_strings=0;
extern string current_block;

/// ---------------------------------------------------------
///                     prefefined tokens
/// ---------------------------------------------------------

static token predefined_tokens[] = {
    {"0",    '0' , 0, 0, 0},
    {"$1",   '$' , 0, 1, 0},
    {"$2",   '$' , 0, 2, 0},
    {"$3",   '$' , 0, 3, 0},
    {"$4",   '$' , 0, 4, 0},
    {"$5",   '$' , 0, 5, 0},
    {"$6",   '$' , 0, 6, 0},
    {"$7",   '$' , 0, 7, 0},
    {"$8",   '$' , 0, 8, 0},
    {"$9",   '$' , 0, 9, 0},
    {"$10",  '$' , 0, 10, 0},
    {"$11",  '$' , 0, 11, 0},
    {"$12",  '$' , 0, 12, 0},
    {"$13",  '$' , 0, 13, 0},
    {"$14",  '$' , 0, 14, 0},
    {"$15",  '$' , 0, 15, 0},
    {"$16",  '$' , 0, 16, 0},
    {"$17",  '$' , 0, 17, 0},
    {"$18",  '$' , 0, 18, 0},
    {"$19",  '$' , 0, 19, 0},
    {"$20",  '$' , 0, 20, 0},
    {"$21",  '$' , 0, 21, 0},
    {"$22",  '$' , 0, 22, 0},
    {"$23",  '$' , 0, 23, 0},
    {"$24",  '$' , 0, 24, 0},
    {"$25",  '$' , 0, 25, 0},
    {"$26",  '$' , 0, 26, 0},
    {"$27",  '$' , 0, 27, 0},
    {"$28",  '$' , 0, 28, 0},
    {"$29",  '$' , 0, 29, 0},
    {"$30",  '$' , 0, 30, 0},
    {"$31",  '$' , 0, 31, 0},
    {"$sp",  '$' , 0, 31, 0},
    {"$pc",  'p' , 0, 0, 0},

    {"#ecause",  '#' , 0, 0, 0},
    {"#epc",     '#' , 0, 1, 0},
    {"#edata",   '#' , 0, 2, 0},
    {"#escratch",'#' , 0, 3, 0},
    {"#counter", '#' , 0, 4, 0},

    {"and",   'A' , 0, 0, 0 },
    {"or",    'A' , 0, 1, 0 },
    {"xor",   'A' , 0, 2, 0 },
    {"add",   'A' , 0, 4, 0 },
    {"sub",   'A' , 0, 5, 0 },
    {"clt",   'A' , 0, 6, 0 },
    {"cltu",  'A' , 0, 7, 0 },
    {"lsl",   'H' , 0, 0, 0 },
    {"lsr",   'H' , 0,-1, 0 },
    {"asr",   'H' , 0,-3, 0 },
    {"ld",    'D' , 0, 2, 0 },
    {"ldb",   'L' , 0, 0, 0 },
    {"ldh",   'L' , 0, 1, 0 },
    {"ldw",   'L' , 0, 2, 0 },
    {"stb",   'S' , 0, 0, 0 },
    {"sth",   'S' , 0, 1, 0 },
    {"stw",   'S' , 0, 2, 0 },
    {"beq",   'B' , 0, 0, 0 },
    {"bne",   'B' , 0, 1, 0 },
    {"blt",   'B' , 0, 2, 0 },
    {"bge",   'B' , 0, 3, 0 },
    {"bltu",  'B' , 0, 4, 0 },
    {"bgeu",  'B' , 0, 5, 0 },
    {"jmp",   'J' , 0, 0, 0 },
    {"mul",   'M' , 0, 0, 0 },
    {"div",   'M' , 0, 4, 0 },
    {"mod",   'M' , 0, 5, 0 },
    {"cfgr",  'C' , 0, 0, 0 },
    {"cfgw",  'C' , 0, 1, 0 },
    {"cjmp",  'C' , 0, 2, 0 },
    {"sys",   'Y' , 0, 0, 0 },

    
    {"jsr",   'j' , 0, 0, 0 },
    {"ret",   'r' , 0, 0, 0 },
    {"dcb",   'd' , 0, 0, 0 },
    {"dch",   'd' , 0, 1, 0 },
    {"dcw",   'd' , 0, 2, 0 },
    {"org",   'o' , 0, 0, 0 },

    {",",     ',' , 0 , 0, 0},
    {"[",     '[' , 0 , 0, 0},
    {"]",     ']' , 0 , 0, 0},
    {":",     ':' , 0 , 0, 0},
    {"=",     '=' , 0 , 0, 0},
    {0,0,0,0,0}
};


/// ---------------------------------------------------------
///                     string_buffer
/// ---------------------------------------------------------

static char* string_buffer;
static int   string_length;
static int   string_alloc;

static void add_to_str(int c) {
    if (string_length+1==string_alloc) {
        string_alloc *= 2;
        resize_array(string_buffer, char, string_alloc);
    }

    string_buffer[string_length++] = c;
    string_buffer[string_length] = 0;
}

/// ---------------------------------------------------------
///                     line_buffer
/// ---------------------------------------------------------

static token** line_buf;
static int     line_alloc;
static int     line_count;

static void add_to_line(token *t) {
    if (line_count+1==line_alloc) {
        line_alloc *= 2;
        resize_array(line_buf, token*, line_alloc);
    }
    line_buf[line_count++] = t;
    line_buf[line_count] = 0;
}


/// ---------------------------------------------------------
///                     hash table
/// ---------------------------------------------------------

static token** hash_table;
static int     hash_size;
static int     hash_count;

static int hash_function(string txt) {
    int ret = 0;
    for(; *txt; txt++)
        ret = (ret*31) + *txt;
    return ret & (hash_size-1);
}

static void hash_add(token *t) {
    if (hash_count*4 > hash_size*3) {
        // table is >75% full - allocate a new table twice the size
        token** old_table = hash_table;
        hash_size *= 2;
        hash_count = 0;
        hash_table = new_array(token*, hash_size);
        for(int k=0; k<hash_size/2; k++)
            if (old_table[k])
                hash_add(old_table[k]);
        free(old_table);
    }

    int slot = hash_function(t->name);
    while(hash_table[slot])
        slot = (slot+1) & (hash_size-1);
    hash_table[slot] = t;
    hash_count++;
}

static token* hash_find(string name) {
    int slot = hash_function(name);
    while(hash_table[slot])
        if (!strcmp(hash_table[slot]->name, name))
            return hash_table[slot];
        else
            slot = (slot+1) & (hash_size-1);
    return 0;
}

static token* new_token(string name, int kind, int value) {
    token* ret = new(token);
    ret->name = strdup(name);
    ret->kind = kind;
    ret->value = value;
    hash_add(ret);

    if (kind=='l') {
        ret->next = all_labels;
        all_labels = ret;
    }

    if (kind=='"') {
        ret->next = all_strings;
        all_strings = ret;
    }

    return ret;
}


static void initialize_hash_table() {
    hash_size = 128;
    hash_count = 0;
    hash_table = new_array(token*, hash_size);
    for(token*t = predefined_tokens; t->name; t++)
        hash_add(t);

    string_alloc = 64;
    string_buffer = new_array(char, string_alloc);

    line_alloc = 32;
    line_buf = new_array(token*, line_alloc);
}

/// ---------------------------------------------------------
///                     open_file
/// ---------------------------------------------------------

static FILE* fh;
static int lookahead;


void open_file(string name) {
    if (hash_table==0)
        initialize_hash_table();

    fh = fopen(name, "r");
    if (fh==0)
        fatal("Cannot open file '%s'", name);

    file_name = name;
    line_number = 0;
    lookahead = fgetc(fh);
}

static int next_char() {
    int ret = lookahead;
    lookahead = fgetc(fh);
    return ret;
}

/// ---------------------------------------------------------
///                     my_atoi
/// ---------------------------------------------------------

static int my_atoi(string s) {
    int sign = 0;
    if (s[0]=='-') {
        s++;
        sign = 1;
    }

    int result = 0;

    int len = strlen(s);
    int last_char = s[len-1];
    if (last_char=='h' || last_char=='H') {
        for(int k=0; k<len-1; k++)
            if (s[k]>='0' && s[k]<='9')
                result = (result<<4) | (s[k]-'0');
            else if (s[k]>='a' && s[k]<='f')
                result = (result<<4) | (s[k]-'a'+10);
            else if (s[k]>='A' && s[k]<='F')
                result = (result<<4) | (s[k]-'A'+10);
            else
                error("Malformed hex literal");
    } else {
        for(int k=0; k<len; k++)
            if (s[k]>='0' && s[k]<='9')
                result = (result*10) + (s[k]-'0');
            else
                error("Malformed decimal literal");
    }

    if (sign)
        return -result;
    else
        return result;
}

/// ---------------------------------------------------------
///                     read string
/// ---------------------------------------------------------

static int next_char_escape() {
    int c = next_char();
    if (c!='\\')
        return c;
    c = next_char();
    if (c=='n')
        return '\n';
    else if (c=='t')
        return '\t';
    else
        return c;
}

static void read_string() {
    add_to_str(next_char()); // the opening "
    while(lookahead!=EOF && lookahead!='"' && lookahead!='\n')
        add_to_str(next_char_escape());
    if (lookahead=='"')
        add_to_str(next_char());
    else
        error("unterminated string");
}



/// ---------------------------------------------------------
///                     next_token
/// ---------------------------------------------------------

token* next_token() {
    token* ret =0;
    do {
        // skip whitespace and comments
        while(lookahead==' ' || lookahead=='\t' || lookahead==';')
            if (lookahead==';')
                while(lookahead!='\n' && lookahead!=EOF)
                    next_char();
            else
                next_char();

        // clear string buffer
        string_length = 0;
        string_buffer[string_length] = 0;


        if (lookahead=='\n' || lookahead==EOF) {
            next_char();
            return 0;

        } else if (isalpha(lookahead) || lookahead=='@' || lookahead=='_') {
            while(isalnum(lookahead) || lookahead=='_' || lookahead=='@' || lookahead=='.')
                add_to_str(next_char());
            ret = hash_find(string_buffer);
            if (ret==0)
                ret = new_token(string_buffer,'l',0);

        } else if (lookahead=='.') {
            for(const char*t=current_block; *t; t++)
                add_to_str(*t);
            add_to_str(next_char());
            while(isalnum(lookahead) || lookahead=='_' || lookahead=='@' || lookahead=='.')
                add_to_str(next_char());
            ret = hash_find(string_buffer);
            if (ret==0)
                ret = new_token(string_buffer,'l',0);

        } else if (lookahead=='$' || lookahead=='#') {
            add_to_str(next_char());
            while(isalnum(lookahead))
                add_to_str(next_char());
            ret = hash_find(string_buffer);

        } else if (lookahead=='\'') {
            add_to_str(next_char());
            add_to_str(next_char());
            if (lookahead=='\'')
                add_to_str(next_char());
            else
                error("unterminated char literal");
            ret = hash_find(string_buffer);
            if (ret==0)
                ret = new_token(string_buffer,'i',string_buffer[1]);

        } else if (isdigit(lookahead) || lookahead=='-') {
            add_to_str(next_char());
            while(isalnum(lookahead))
                add_to_str(next_char());
            ret = hash_find(string_buffer);
            if (ret==0)
                ret = new_token(string_buffer,'i',my_atoi(string_buffer));

        } else if (lookahead=='"') {
            read_string();
            ret = hash_find(string_buffer);
            if (ret==0)
                ret = new_token(string_buffer,'"',0);


        } else {
            add_to_str(next_char());
            ret = hash_find(string_buffer);
        }

        if (ret==0)
            error("Invalid token '%s'", string_buffer);
    } while(ret==0);
    return ret;
}


/// ---------------------------------------------------------
///                     line_buffer
/// ---------------------------------------------------------

token** read_line() {
    line_count = 0;
    line_buf[0] = 0;
    line_number++;

    if (lookahead==EOF)
        return 0;

    token *t;
    while((t=next_token()))
        add_to_line(t);

    return line_buf;
}




