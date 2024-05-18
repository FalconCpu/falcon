// The F32 CPU 
//
// Instructions are all 32 bits lons, and are split into the following fields
//
// 109876 543 21098 76543 21098765 43210
// KKKKKK III DDDDD AAAAA CCCCCCCC BBBBB
//
// KKKKKK = Main 'kind' of instruction. Currently all instructions begin with 01
// III    = Combined with K gives the exact instruction
// DDDDD  = Register to write result of instruction to
// AAAAA  = Register number of first argument
// CCCCC  = Literal values or further details of instruction
// BBBBB  = Register number of second argument
//
// Literal values are built by combining fields together. 
// They are sign extended - C field is always the MSB section of the literal
// N13    = CCCCCCCCBBBBB
// N13D   = CCCCCCCCDDDDD
// N21    = CCCCCCCCIIIAAAAABBBBB

// Values for the 'K' field
`define INSTR_ALU_REG   6'h10
`define INSTR_ALU_LIT   6'h11
`define INSTR_LOAD      6'h12
`define INSTR_STORE     6'h13
`define INSTR_BRANCH    6'h14
`define INSTR_JUMP      6'h15
`define INSTR_JUMP_REG  6'h16
`define INSTR_LDU       6'h17
`define INSTR_LDPC      6'h18
`define INSTR_CFG       6'h19
`define INSTR_MUL       6'h1A
`define INSTR_MUL_LIT   6'h1B

// Values for K & I fields combined (drop the first 2'b01 from the K field)
`define OP_AND      7'h00
`define OP_OR       7'h01
`define OP_XOR      7'h02
`define OP_SHIFT    7'h03
`define OP_ADD      7'h04
`define OP_SUB      7'h05
`define OP_CLT      7'h06
`define OP_CLTU     7'h07
`define OP_AND_I    7'h08
`define OP_OR_I     7'h09
`define OP_XOR_I    7'h0A
`define OP_SHIFT_I  7'h0B
`define OP_ADD_I    7'h0C
`define OP_SUB_I    7'h0D
`define OP_CLT_I    7'h0E
`define OP_CLTU_I   7'h0F
`define OP_LDB      7'h10
`define OP_LDH      7'h11
`define OP_LDW      7'h12
`define OP_STB      7'h18
`define OP_STH      7'h19
`define OP_STW      7'h1A
`define OP_BEQ      7'h20
`define OP_BNE      7'h21
`define OP_BLT      7'h22
`define OP_BGE      7'h23
`define OP_BLTU     7'h24
`define OP_BGEU     7'h25
`define OP_JMP      7'h28
`define OP_RETE     7'h29
`define OP_JMPR     7'h30
`define OP_LDU      7'h38
`define OP_LDPC     7'h40
`define OP_CFGLOAD  7'h48
`define OP_CFGSET   7'h49
`define OP_CFGCLR   7'h4A
`define OP_CFGJMP   7'h4B
`define OP_MUL      7'h50
`define OP_DIVS     7'h54
`define OP_MODS     7'h55
`define OP_DIVU     7'h56
`define OP_MODU     7'h57
`define OP_MUL_I    7'h58
`define OP_DIVS_I   7'h5c
`define OP_MODS_I   7'h5d
`define OP_DIVU_I   7'h5e
`define OP_MODU_I   7'h5f

// Possible sources for arguments 
`define SRC_ZERO  2'h0
`define SRC_REG   2'h1
`define SRC_PC    2'h2      // Only for 'A' args
`define SRC_LIT   2'h2      // Only for 'B' args

// config register numbers
`define CFG_ECAUSE    32'h0
`define CFG_EPC       32'h1
`define CFG_EDATA     32'h2
`define CFG_ESCRATCH  32'h3
`define CFG_COUNTER   32'h4

