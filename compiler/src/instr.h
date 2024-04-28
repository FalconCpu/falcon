#include "fpl.h"

// =================================================================================
//                                Instr
// =================================================================================

// Represents one "Three Address Code" (TAC) instruction in the program


typedef enum {
    ALU_AND_I,
    ALU_OR_I,
    ALU_XOR_I,
    ALU_LSL_I,
    ALU_LSR_I,
    ALU_ASR_I,
    ALU_ADD_I,
    ALU_SUB_I,
    ALU_MUL_I,
    ALU_DIV_I,
    ALU_MOD_I,
    ALU_EQ_I,
    ALU_NEQ_I,
    ALU_LT_I,
    ALU_LTE_I,
    ALU_GT_I,
    ALU_GTE_I,

    ALU_BEQ_I,
    ALU_BNE_I,
    ALU_BLT_I,
    ALU_BLE_I,
    ALU_BGT_I,
    ALU_BGE_I,

    ALU_ADD_F,
    ALU_SUB_F,
    ALU_MUL_F,
    ALU_DIV_F,
    ALU_MOD_F,
    ALU_EQ_F,
    ALU_NEQ_F,
    ALU_LT_F,
    ALU_LTE_F,
    ALU_GT_F,
    ALU_GTE_F,

    ALU_ADD_S,
    ALU_EQ_S,
    ALU_NEQ_S,
    ALU_LT_S,
    ALU_LTE_S,
    ALU_GT_S,
    ALU_GTE_S,

    ALU_AND_B,
    ALU_OR_B
} AluOp;

typedef enum {
    INSTR_NOP,
    INSTR_MOV,
    INSTR_ALU,
    INSTR_LOAD,
    INSTR_STORE,
    INSTR_BRANCH,
    INSTR_JUMP,
    INSTR_LABEL,
    INSTR_CALL,
    INSTR_LEA,
    INSTR_CHK,
    INSTR_START,
    INSTR_END
} InstrKind;

struct Instr  {
    int       index;      // location of the instruction in the program
    InstrKind kind;       // Broad classification of the operation ALU/BRANCH/LOAD/STORE etc
    AluOp     op;         // Detailed classification of operation (eg ADD_I, SUB_I)
    Symbol    dest;       // Destination operand (if applicable)
    Symbol    a;          // First data operand  (if applicable)
    Symbol    b;          // Second data operand  (if applicable)
};

#define REG_ARGS 1
#define REG_RESULT 8
#define REG_CALLEE_SAVE 9
#define REG_MAX 27
#define REG_SP 31
#define REG_LINK 30
#define REG_GLOBALS 28


extern String alu_names[];

Instr new_Instr(InstrKind kind, AluOp op, Symbol dest, Symbol a, Symbol b);

// convert an instr into a human readable form
String instr_toString(Instr this);