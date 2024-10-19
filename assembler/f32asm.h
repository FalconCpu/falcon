typedef const char* string;
typedef struct token token;
typedef struct reference reference;

/// --------------------------------------------------
///                   token
/// --------------------------------------------------

struct token {
    string name;
    short  kind;
    short  flags;
    int    value;
    token* next;
};

#define FLAG_DEFINED 1

#define KIND_REGISTER '$'
#define KIND_INTEGER  'i'
#define KIND_LABEL    'l'
#define KIND_ALU      'A'
#define KIND_LD       'D'
#define KIND_BRANCH   'B'
#define KIND_LOAD     'L'
#define KIND_STORE    'M'
#define KIND_JUMP     'J'
#define KIND_SYSCALL  'S'


/// --------------------------------------------------
///                   reference
/// --------------------------------------------------

struct reference {
    token *token;
    int    location;
    string file_name;
    int    line_number;
    reference *next;
};


/// --------------------------------------------------
///                   lex
/// --------------------------------------------------

void open_file(string filename);
token* next_token();
token** read_line();


/// --------------------------------------------------
///                   assemble
/// --------------------------------------------------

void assemble_file(string file_name);
void output_file(string file_name);

/// --------------------------------------------------
///                   disassemble
/// --------------------------------------------------

void read_file(string file_name);
void disassemble();

/// --------------------------------------------------
///                   util
/// --------------------------------------------------

void fatal(string message, ...) __attribute((noreturn));
void error(string message,...);
void* my_malloc(int size);
void* my_realloc(void *old, int new_size);
#define new(T) (T*)my_malloc(sizeof(T))
#define new_array(T,N) (T*)my_malloc((N)*sizeof(T))
#define resize_array(A,T,N) A=my_realloc(A,(N)*sizeof(T))
#define assert(A) if (!(A)) fatal("Failed assertion %s",#A)


