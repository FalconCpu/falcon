#include <fstream>
#include <vector> 
#include <ctype.h>
#include "fpl.h"
#include <unordered_map>
#include <string>
using std::unordered_map;
using std::string;  
using std::string_view;

// ==========================================
// global variables to communicate with parser
// ==========================================

typedef union YYSTYPE {
    const string*   string;
    Ast*      ast;
} YYSTYPE;

extern Location      yylloc;
extern YYSTYPE       yylval;

static unordered_map<string, int> hash_table;
static string buffer;

static string empty_string = "";

// ==========================================
//                hash table
// ==========================================


const string tokenKinds[] = {
    "<end of file>",
    "<end of line>",
    "<indent>",
    "<outdent>",
    "<identifier>",
    "<int literal>",
    "<real literal>",
    "<string literal>",
    "<char literal>",
    "+",
    "-",
    "*",
    "/",
    "%",
    "&",
    "|",
    "^",
    "=",
    "!=",
    "<",
    "<=",
    ">",
    ">=",
    "not",
    "and",
    "or",
    ".",
    "->",
    ",",
    ":",
    "(",
    "[",
    "{",
    ")",
    "]",
    "}",
    "val",
    "var",
    "if",
    "else",
    "end",
    "while",
    "repeat",
    "until",
    "class",
    "enum",
    "return",
    "fun",
    "<error>"
};

static void initialize() {
    if (hash_table.size()!=0) 
        return ;

    if (tokenKinds[TOKEN_ERROR]!="<error>")
        fatal("Internal Error: tokenKinds[TOKEN_ERROR] is not %s", tokenKinds[TOKEN_ERROR].c_str());

    for(int k=0; k<TOKEN_ERROR; k++)    
        hash_table[tokenKinds[k]] = k;

}

string get_token_name(int token) {
    return tokenKinds[token];
}

// ==========================================
//                open file
// ==========================================

static string current_filename;
static int line;
static int column;
static int lookahead;
static std::ifstream inFile;
static int at_start_of_line;
static int line_continues;
static std::vector<int> indent_stack;

void open_file(string filename) {
    initialize();
    inFile.open(filename);
    if (!inFile.is_open())
        fatal("cannot open file: %s", filename.c_str());

    current_filename = filename;
    line = 1;
    column = 1;
    at_start_of_line = 1;
    line_continues = 1;
    lookahead = inFile.get();
    indent_stack = std::vector<int>{1};
}

// ==========================================
//                next char
// ==========================================

static int next_char() {
    int c = lookahead;
    lookahead = inFile.get();
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

static string read_word() {
    buffer.clear();
    while (isalnum(lookahead) || lookahead=='_')
        buffer.push_back(next_char());
    return buffer;
}

// ==========================================
//                read punctuation
// ==========================================

static string read_punctuation() {
    buffer.clear();
    int c = next_char();
    buffer.push_back(c);
    if ( (c=='!' && lookahead=='=') ||
         (c=='<' && lookahead=='=') ||
         (c=='>' && lookahead=='=') ||
         (c=='-' && lookahead=='>') )
        buffer.push_back(next_char() );
    return buffer;
}


// ==========================================
//      skip whitespace and comments
// ==========================================

static void skip_whitespace() {
    while (lookahead==' ' || lookahead=='\t' || lookahead=='\r'|| lookahead=='#' || (lookahead=='\n' && line_continues)) {
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
    yylval.string = &empty_string;

    buffer.clear();
    int ret = 0;

    if (lookahead==EOF) {
        if (!at_start_of_line)
            ret = TOKEN_EOL;
        else if (indent_stack.size()>1) {
            indent_stack.pop_back();
            ret = TOKEN_OUTDENT;
        } else
            ret = TOKEN_EOF;
    
    } else if (at_start_of_line && column>indent_stack.back()) {
        indent_stack.push_back(column);
        ret = TOKEN_INDENT;

    } else if (at_start_of_line && column<indent_stack.back()) {
        if (indent_stack.size()==1) {
            error(yylloc, "invalid indentation");
            indent_stack.back() = column;
        } else 
            indent_stack.pop_back();
        if (column>indent_stack.back()) {
            error(yylloc, "unindent does not match any outer indentation level");
            indent_stack.back() = column;
        }
        ret = TOKEN_OUTDENT;

    } else if (lookahead=='\n') {
        next_char();
        ret = TOKEN_EOL;

    } else if (isdigit(lookahead)) {
        read_word();
        auto he = hash_table.try_emplace(buffer, TOKEN_INTLIT);
        ret = he.first->second;
        yylval.string = &(he.first->first);
    
    } else if (isalpha(lookahead)) {
        read_word();
        auto he = hash_table.try_emplace(buffer, TOKEN_IDENTIFIER);
        ret = he.first->second;
        yylval.string = &(he.first->first);

    } else {
        read_punctuation();
        hash_table.try_emplace(buffer, TOKEN_ERROR);
        ret = hash_table[buffer];
        if (ret==TOKEN_ERROR)
            error(yylloc, "invalid character: %s", buffer.c_str());
    }

    line_continues = (ret>=TOKEN_PLUS && ret<=TOKEN_OPENCL) || (ret==TOKEN_EOL);
    at_start_of_line = (ret==TOKEN_EOL || ret==TOKEN_OUTDENT);
    return ret;
}


