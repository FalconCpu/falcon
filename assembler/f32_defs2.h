// -------------------------------------------------------------
//                          CPU values
// -------------------------------------------------------------

#define KIND_ALU   0x10
#define KIND_ALUI  0x11
#define KIND_LD    0x12
#define KIND_ST    0x13
#define KIND_BRA   0x14
#define KIND_JMP   0x15
#define KIND_JMPR  0x16
#define KIND_LDU   0x17
#define KIND_LDPC  0x18
#define KIND_MULT  0x19
#define KIND_CFG   0x1A
#define KIND_MULTI 0x1B

#define ALU_AND    0x00
#define ALU_OR     0x01
#define ALU_XOR    0x02
#define ALU_SHIFT  0x03
#define ALU_ADD    0x04
#define ALU_SUB    0x05
#define ALU_CLT    0x06
#define ALU_CLTU   0x07

#define SHIFT_LSL    0
#define SHIFT_ASR    -1
#define SHIFT_LSR    -3

#define COMP_EQ      0
#define COMP_NEQ     1
#define COMP_LT      2
#define COMP_GE      3
#define COMP_LTU     4
#define COMP_GEU     5

#define MULT_MULT    0
#define MULT_DIVS    4
#define MULT_MODS    5
#define MULT_DIVU    6
#define MULT_MODU    7

#define CFG_VERSION  0
#define CFG_STATUS   1
#define CFG_EPC      2
#define CFG_ECAUSE   3
#define CFG_EDATA    4
#define CFG_ESTATUS  5
#define CFG_ESCRATCH 6

#define CFG_CMD_LD    0
#define CFG_CMD_SET   1
#define CFG_CMD_CLR   2
#define CFG_CMD_SYS   3
#define CFG_CMD_RETE  4

#define EXCEPTION_ILLEGAL_INSTRUCTION 1
#define EXCEPTION_MISALIGNED_LOAD     2
#define EXCEPTION_MISALIGNED_STORE    3
#define EXCEPTION_SYSCALL             4
