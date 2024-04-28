#include "fpl.h"

// ==========================================================================
//                              Tokens
// ==========================================================================
// Tokens represent a unit of text

enum TokenKind {
    TOK_EOF,        
    TOK_EOL,        
    TOK_INDENT,     
    TOK_DEDENT,     
    TOK_ID,         
    TOK_INTLIT,     
    TOK_CHARLIT,    
    TOK_STRLIT,     
    TOK_REALLIT,    
    TOK_PLUS,       
    TOK_MINUS,      
    TOK_STAR,       
    TOK_SLASH,      
    TOK_PERCENT,    
    TOK_AMPERSAND,  
    TOK_BAR,        
    TOK_CARAT,      
    TOK_EQ,         
    TOK_NEQ,        
    TOK_LT,         
    TOK_LTE,        
    TOK_GT,         
    TOK_GTE,        
    TOK_AND,        
    TOK_OR,         
    TOK_DOT,        
    TOK_COMMA,      
    TOK_COLON,      
    TOK_LEFT,       
    TOK_RIGHT,      
    TOK_ARROW,      
    TOK_OPENB,      
    TOK_OPENSQ,     
    TOK_OPENCL,     
    TOK_CLOSEB,     
    TOK_CLOSESQ,    
    TOK_CLOSECL,    
    TOK_VAL,        
    TOK_VAR,        
    TOK_IF,         
    TOK_END,        
    TOK_ELSE,       
    TOK_ELSIF,
    TOK_REPEAT,     
    TOK_UNTIL,      
    TOK_WHILE,      
    TOK_RETURN,     
    TOK_FUN,        
    TOK_STRUCT,     
    TOK_QMARK,      
    TOK_EMARK,      
    TOK_QMARKDOT,   
    TOK_QMARKCOLON, 
    TOK_NEW,        
    TOK_IN,         
    TOK_TO,         
    TOK_FOR,        
    TOK_EXTERN,     
    TOK_NOT,     
    TOK_ERROR      
} ;


struct Token {
    Location  location;
    TokenKind kind;
    String    text;
};



// open a file and prepare to read tokens from it
void open_file(String file_name);

// Map TokenKinds to names
extern String token_kind_names[];

// Read the next token from the file
Token read_token();

// Create a unique pointer for any string value. This allows strings to be compared
// by pointer equality, rather than requiring strcmp
//
// IMPORTANT: whenever you encounter a string from an external source (for example
// during lexing) pass it through unique_string() before storing it in the AST
// or anywhere. This ensures the above invariant, and allows memory to be managed.
String unique_string(String s);



