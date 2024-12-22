typedef const char * string;
typedef struct Token* Token;
typedef struct Reference Reference;

#define KIND_ALU   0x10
#define KIND_ALUI  0x11
#define KIND_LD    0x12
#define KIND_ST    0x13
#define KIND_BRA   0x14
#define KIND_JMP   0x15
#define KIND_JMPR  0x16
#define KIND_LDU   0x17
#define KIND_LDPC  0x18
#define KIND_MUL   0x19
#define KIND_MULI  0x1A
#define KIND_CFG   0x1B

#define FLAG_DEFINED   0x01

#define FILE_FORMAT_HEX  0
#define FILE_FORMAT_BIN  1
#define FILE_FORMAT_HUNK 2

// ----------------------------------------------------
//                        token.c
// ----------------------------------------------------

struct Token {
    short    kind;
    short    flags;
    string   text;
    int      value;
    struct Token* next;
};

// Perpare the Lexer to read from a specified file
void open_file(string filename);

// Read a line from the file and return an array of tokens
Token* read_line();

// ----------------------------------------------------
//                        reference
// ----------------------------------------------------

struct Reference {
    int line_number;  // Line number where the label is referenced
    int address;      // Address or PC of the instruction
    Token label;      // The label being referenced (a Token object)
} ;


// ----------------------------------------------------
//                        assemble.c
// ----------------------------------------------------

void initialize_assembler();
void assemble_file(string filename);
void output_result(string filename, int format);

// ----------------------------------------------------
//                        disassemble.c
// ----------------------------------------------------

void load_labels(string filename);
string find_label(int addr);
char *disassemble_line(int op, int pc);
void disassemble_program(int *program, int len);

// ----------------------------------------------------
//                        execute.c
// ----------------------------------------------------

void execute();


// ----------------------------------------------------
//                        util.c
// ----------------------------------------------------

void fatal(string msg,...) __attribute__((noreturn));
void error(string msg,...);
void* my_malloc(size_t size);
void* my_realloc(void* ptr, size_t size);
#define assert(A) if (!(A)) fatal("Assertion failed: %s", #A)
