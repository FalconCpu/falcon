#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "fpl.h"

// ==========================================
// global variables to communicate with parser
// ==========================================

extern Location yylloc;
extern String   yylval;

// ==========================================
//                hash table
// ==========================================

typedef struct {
    String    key;
    int       kind;
} HashEntry;

static HashEntry* hash_table;
static size_t     hash_table_size;
static size_t     hash_table_count;
static int        is_initialized = 0;

static int hash_function(String s) {
    int ret = 0;
    for (; *s; s++)
        ret = 31*ret + *s;
    return ret & (hash_table_size-1);
}

static HashEntry* hash_table_lookup(String s) {
    int h = hash_function(s);
    while (hash_table[h].key!=0) {
        if (!strcmp(hash_table[h].key, s))
            return &hash_table[h];
        h = (h+1) & (hash_table_size-1);
    }
    return 0;
}

static HashEntry* hash_table_add(String s, int kind) {
    if (hash_table_count*4 > hash_table_size*3) {
        HashEntry* old_hash_table = hash_table;
        hash_table_size *= 2;
        hash_table = new_array(HashEntry, hash_table_size);
        for(int k=0; k<hash_table_size/2; k++)
            if (old_hash_table[k].key!=0)
                hash_table_add(old_hash_table[k].key, old_hash_table[k].kind);
        free(old_hash_table);
    }

    int h = hash_function(s);
    while (hash_table[h].key!=0)
        h = (h+1) & (hash_table_size-1);
    hash_table[h].key = s;
    hash_table[h].kind = kind;
    hash_table_count++;
    return &hash_table[h];
}

static HashEntry* hash_table_get_or_put(String s, int kind) {
    HashEntry *ret = hash_table_lookup(s);
    if (ret==0)
        ret = hash_table_add(_strdup(s),kind);
    return ret;
}

static HashEntry predefined_tokens[] = {
    {"<end of file>", TOKEN_EOF},
    {"<end of line>", TOKEN_EOL},
    {"<indent>", TOKEN_INDENT},
    {"<outdent>", TOKEN_OUTDENT},
    {"<identifier>", TOKEN_IDENTIFIER},
    {"<int literal>", TOKEN_INTLIT},
    {"<real literal>", TOKEN_REALLIT},
    {"<string literal>", TOKEN_STRINGLIT},
    {"<char literal>", TOKEN_CHARLIT},
    {"+", TOKEN_PLUS},
    {"-", TOKEN_MINUS},
    {"*", TOKEN_STAR},
    {"/", TOKEN_SLASH},
    {"%", TOKEN_PERCENT},
    {"&", TOKEN_AMP},
    {"|", TOKEN_BAR},
    {"^", TOKEN_CARAT},
    {"=", TOKEN_EQ},
    {"!=", TOKEN_NEQ},
    {"<", TOKEN_LT},
    {"<=", TOKEN_LTE},
    {"", TOKEN_GT},
    {">", TOKEN_GTE},
    {">=", TOKEN_NOT},
    {"and", TOKEN_AND},
    {"or", TOKEN_OR},
    {"/", TOKEN_DOT},
    {"->", TOKEN_ARROW},
    {",", TOKEN_COMMA},
    {":", TOKEN_COLON},
    {"(", TOKEN_OPENB},
    {"[", TOKEN_OPENSQ},
    {"{", TOKEN_OPENCL},
    {")", TOKEN_CLOSEB},
    {"]", TOKEN_CLOSESQ},
    {"}", TOKEN_CLOSECL},
    {"val", TOKEN_VAL},
    {"var", TOKEN_VAR},
    {"if", TOKEN_IF},
    {"else", TOKEN_ELSE},
    {"end", TOKEN_END},
    {"while", TOKEN_WHILE},
    {"repeat", TOKEN_REPEAT},
    {"until", TOKEN_UNTIL},
    {"class", TOKEN_CLASS},
    {"enum", TOKEN_ENUM},
    {"return", TOKEN_RETURN},
    {"<error>", TOKEN_ERROR}
};

// ==========================================
//               token_kind_name
// ==========================================

String token_kind_name(int kind) {
    for(int k=0; predefined_tokens[k].kind!=TOKEN_ERROR; k++)
        if (predefined_tokens[k].kind==kind)
            return predefined_tokens[k].key;
    return "<unknown>";
}

// ==========================================
//                char buffer
// ==========================================

static char* buffer;
static size_t buffer_size;
static size_t buffer_length;

static void buffer_append(char c) {
    if (buffer_length+1==buffer_size) {
        buffer_size *= 2;
        buffer = my_realloc(buffer, buffer_size);
    }
    buffer[buffer_length++] = c;
    buffer[buffer_length] = 0;
}

static void buffer_clear() {
    buffer_length = 0;
    buffer[0] = 0;
}

// ==========================================
//                initialize
// ==========================================
// Initializes the character buffer and the hash table.

static void initialize() {
    buffer_size = 64;
    buffer = my_malloc(buffer_size);
    buffer_length = 0;
 
    hash_table_size = 1024;
    hash_table = new_array(HashEntry, hash_table_size);
    hash_table_count = 0;
    for (int k=0; predefined_tokens[k].kind!=TOKEN_ERROR; k++)
        hash_table_add(predefined_tokens[k].key, predefined_tokens[k].kind);
    is_initialized = 1;
}

// ==========================================
//                open file
// ==========================================

static String current_filename;
static int line;
static int column;
static int lookahead;
static FILE* fp;

void open_file(String filename) {
    if (!is_initialized)
        initialize();

    errno_t err = fopen_s(&fp, filename, "r");
    if (err != 0)
        fatal("cannot open file: %s", filename);

    current_filename = filename;
    line = 1;
    column = 1;
    lookahead = getc(fp);

}

// ==========================================
//                next char
// ==========================================

static int next_char() {
    int c = lookahead;
    lookahead = getc(fp);
    if (c=='\n') {
        line++;
        column = 1;
    } else
        column++;
    return c;
}

// ==========================================
//                read word
// ==========================================

static String read_word() {
    buffer_clear();
    while (isalnum(lookahead) || lookahead=='_')
        buffer_append(next_char());
    return buffer;
}

// ==========================================
//                read punctuation
// ==========================================

static String read_punctuation() {
    buffer_clear();
    int c = next_char();
    buffer_append(c);
    if ( (c=='!' && lookahead=='=') ||
         (c=='<' && lookahead=='=') ||
         (c=='>' && lookahead=='=') ||
         (c=='-' && lookahead=='>') )
        buffer_append( next_char() );
    return buffer;
}


// ==========================================
//      skip whitespace and comments
// ==========================================

static void skip_whitespace() {
    while (lookahead==' ' || lookahead=='\t' || lookahead=='\r'|| lookahead=='#') {
        if (lookahead=='#') 
            while (lookahead!='\n' && lookahead!=EOF)
                next_char();
        else
            next_char();
    }
}

// ==========================================
//                yylex
// ==========================================

int yylex() {
    skip_whitespace();
    yylloc.filename = current_filename;
    yylloc.line = line;
    yylloc.column = column;

    HashEntry *he = 0;

    if (lookahead==EOF) {
        he = hash_table_lookup("<end of file>");
    
    } else if (lookahead=='\n') {
        next_char();
        he = hash_table_lookup("<end of line>");

    } else if (isdigit(lookahead)) {
        String s = read_word();
        he = hash_table_get_or_put(s,TOKEN_INTLIT);
    
    } else if (iswalpha(lookahead)) {
        String s = read_word();
        he = hash_table_get_or_put(s, TOKEN_IDENTIFIER);

    } else {
        String s = read_punctuation();
        he = hash_table_get_or_put(s, TOKEN_ERROR);
    }

    if (he==0)
        fatal("null hashEntry in yylex");

    yylval = he->key;
    return he->kind;
}


