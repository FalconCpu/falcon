#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "f32asm.h"
#include "f32_defs.h"
//#include "util.h"

typedef const char* string;


#define MEM_SIZE 0x10000
int prog[MEM_SIZE/4];
int prog_length;

int patt_ram[16384];



token* all_labels;
int line_number;
string file_name;

#define RAM_SIZE 64*1024*1024

static int* ram;
static int finished;

static int debug=0;   // 0 = nothing, 1=register writes, 2=instructions executed
extern char* disassemble_line(int addr);

int cycle_count = 0;

static int regs[32];
int pc;
static int supervisor = 1;
static int epc = 0;
static int ecause = 0;
static int edata = 0;
static int estatus = 0;
static int escratch = 0;
static int null_as_exception=0;

FILE *debug_trace;
FILE *reg_trace;
FILE *uart_out;
static void exception(int cause, int data);

static int keyboard_scancodes[] = { 0x1A,       // Press 'z'
                                    0xF0, 0x1A, // release 'z'
                                    0x12,       // Press shift
                                    0x4B,       // press 'L'
                                    0xF0, 0x4B, // release 'L',
                                    0x46,       // press '9'
                                    0xF0, 0x46, // release '9'
                                    0xF0, 0x12, // release shift,
                                    0x46,       // press '9'
                                    0xF0, 0x46, // release '9'
                                    0x42,       // press 'k'
                                    0xF0, 0x42  // release 'k'
                                    -1};
static int read_keyboard() {
    static int keyboard_index = 0;
    if (keyboard_scancodes[keyboard_index]==-1)
        return -1;
    else
        return keyboard_scancodes[keyboard_index++];
}



static void setReg(int reg, int value) {
    if (reg!=0  && !null_as_exception) {
        fprintf(reg_trace,"$%2d = %08x\n",reg,value);
        if (debug>=2)
            fprintf(debug_trace,"$%-2d=%08x",reg,value);
        regs[reg] = value;
    }
}

static int aluOp(int op, int a, int b, int c) {
    switch(op) {
        case ALU_AND: return a & b;
        case ALU_OR: return a | b;
        case ALU_XOR: return a ^ b;
        case ALU_SHIFT: if (c==SHIFT_LSL)
                    return a << b;
                else if (c==SHIFT_ASR)
                    return a >> b;
                else if (c==SHIFT_LSR)
                    return ((unsigned) a) >> b;
        case ALU_ADD: return a + b;
        case ALU_SUB: return a - b;
        case ALU_CLT: return (a < b);
        case ALU_CLTU: return ((unsigned)a) & ((unsigned)b);
    }
    return 0;
}

static void writeMemMappedReg(unsigned int addr, int value) {
    switch(addr) {
        case 0xE0000000: // 7 seg display
            printf("7SEG %06X", value & 0xffffff);
            break;

        case 0xE0000004: // leds
            printf("LEDS %03X", value & 0x3FF);
            break;

        case 0xE0000014: // uart tx
            fprintf(uart_out, "%c", value & 0xFF);
            printf("%c", value & 0xFF);
            break;

        case 0xE000001C: 
            printf("SCREEN BLANK %3X", value & 0x3FF);
            break;
    }
}

static int readMemMappedReg(unsigned int addr) {
    switch(addr) {
        case 0xE0000014: // uart tx
            return 100;  // claim there is space in the fifo
        
        case 0xE0000028:  // Keyboard fifo - report no keys
            return read_keyboard();

        default:
            return 0;
    }
}

static int readMem(unsigned int addr) {
    // TO DO - alignment check
    if (addr>=0 && addr<=(4*RAM_SIZE))
        return ram[addr>>2];
    else if (addr>=0xE1000000 && addr<0xE1010000)
        return patt_ram[(addr & 0xFFFF)>>2];
    else if (addr>=0xFFFF0000)
        return prog[(addr & 0xFFFF)>>2];
    else if (addr >= 0xE0000000 && addr<=0xE000FFFF)
        return readMemMappedReg(addr);
    else
        return 0;
}

static void writeMem(unsigned int addr, int mask, int value) {
    // TO DO - alignment check

    if (addr>=0 && addr<=(4*RAM_SIZE)) {
        int a2 = addr>>2;
        ram[a2] = (ram[a2] & ~mask) | (value & mask);
        if (debug>=2)
            fprintf(debug_trace,"[%08x]=%x",addr,ram[a2]);
    } else if (addr >= 0xE0000000 && addr<=0xE000FFFF) {
        writeMemMappedReg(addr, value);
    } else if (addr >= 0xE1000000 && addr<0xE1010000) {
        int a2 = (addr&0xffff) >> 2;
        patt_ram[a2] = (patt_ram[a2] & ~mask) | (value & mask);
        if (debug>=2)
            fprintf(debug_trace,"[%08x]=%x",addr,patt_ram[a2]);
    } else if (addr >= 0xFFFF0000) {
        int a2 = (addr&0xffff) >> 2;
        prog[a2] = (prog[a2] & ~mask) | (value & mask);
        if (debug>=2)
            fprintf(debug_trace,"[%08x]=%x",addr,prog[a2]);
    }
}

static int read(int addr, int size) {
    if ((size==1 && addr&1) || (size==2 && addr&3))
        exception(EXCEPTION_MISALIGNED_LOAD,addr);

    int lsb = (addr & 3) *8;
    int mem = readMem(addr);
    switch(size) {
        case 0:  mem = (mem>>lsb) & 0xff;
                 return (mem & 0x80) ? mem | 0xffffff00 : mem; 
        case 1:  mem = (mem>>lsb) & 0xffff;
                 return (mem & 0x8000) ? mem | 0xffff0000 : mem; 
        case 2:  return mem;
        default: return 0;
    }
}

static void write(int addr, int size, int value) {
    if ((size==1 && addr&1) || (size==2 && addr&3))
        exception(EXCEPTION_MISALIGNED_STORE,addr);

    int lsb = (addr & 3) * 8;
    switch(size) {
        case 0:  writeMem(addr&0xfffffffc, 0xff<<lsb, (value&0xff)<<lsb); break;
        case 1:  writeMem(addr&0xfffffffc, 0xffff<<lsb, (value&0xffff)<<lsb); break;
        case 2:  writeMem(addr&0xfffffffc, 0xffffffff, value); break;
    }
}


static int comp(int op, int a, int b) {
    switch(op) {
        case COMP_EQ:   return a==b;
        case COMP_NEQ:   return a!=b;
        case COMP_LT:   return a<b;
        case COMP_GE:   return a>=b;
        case COMP_LTU:   return ((unsigned)a) < ((unsigned)b);
        case COMP_GEU:   return ((unsigned)a) >= ((unsigned)b);
        default: return 1;
    }
}

static int mult_op(int op, int a, int b) {
    switch(op) {
        case MULT_MULT:    return a * b;
        case MULT_DIVS:    return (b==0) ? -1 : a / b;
        case MULT_MODS:    return (b==0) ? a  : a % b;
        case MULT_DIVU:    return (b==0) ? -1 : (unsigned)a / (unsigned)b;
        case MULT_MODU:    return (b==0) ? a  :  (unsigned)a % (unsigned)b;
        default:           return 0;
    }
}

static int read_cfg(int reg) {
    switch(reg) {
        case CFG_VERSION:    return 1;
        case CFG_STATUS:     return supervisor;
        case CFG_EPC:        return epc;
        case CFG_ECAUSE:     return ecause;
        case CFG_EDATA:      return edata;
        case CFG_ESTATUS:    return estatus;
        case CFG_ESCRATCH:   return escratch;
        default:             return 0;
    }
}

static void write_cfg(int reg, int value) {
    switch(reg) {
        case CFG_VERSION:    break;
        case CFG_STATUS:     break; // DON'T allow write of supervisor through here
        case CFG_EPC:        epc = value; break;
        case CFG_ECAUSE:     ecause = value & 0xf; break;
        case CFG_EDATA:      edata = value; break;
        case CFG_ESTATUS:    estatus = value & 1; break;
        case CFG_ESCRATCH:   escratch = value; break;
        default:             return;
    }
}

static void cfg_cmd(int cmd, int dest_reg, int cfg_reg, int value) {
    int x;

    switch(cmd) {
        case CFG_CMD_LD: 
            x = read_cfg(cfg_reg);
            setReg(dest_reg, x);
            write_cfg(cfg_reg, value);
            break;

        case CFG_CMD_SET:
            x = read_cfg(cfg_reg);
            setReg(dest_reg, x);
            write_cfg(cfg_reg, x | value);
            break;

        case CFG_CMD_CLR:
            x = read_cfg(cfg_reg);
            setReg(dest_reg, x);
            write_cfg(cfg_reg, x & ~value);
            break;

        case CFG_CMD_JMP:
            pc = read_cfg(cfg_reg);
            break;

        case CFG_CMD_SYS:
            epc = pc;
            pc = 0xFFFF0004;
            ecause = EXCEPTION_SYSCALL;
            edata  = cfg_reg;
            if (debug>=2)
                fprintf(debug_trace,"SYS %d",cfg_reg);
            break;

        default:    
            break;
    }
}

static void exception(int cause, int data) {
    if (debug>=2)
        fprintf(debug_trace,"EXC %x %x",cause,data);
    if (cause==EXCEPTION_SYSCALL)
        epc = pc;
    else
        epc = pc-4; 
    pc = 0xffff0004;
    ecause = cause;
    edata = data;
    null_as_exception = 1;
}

static void executeInstruction(int instr) {
    if (debug>=2)
         fprintf(debug_trace,"%-5d %08X: %-30s   ",cycle_count, pc-4, disassemble_line(pc-4));
    cycle_count++;
    null_as_exception = 0;

    int k = (instr>>26) & 0x3f;
    int i = (instr>>23) & 7;
    int d = (instr>>18) & 31;
    int a = (instr>>13) & 31;
    int c = (instr>>5)  & 0xff; if (c&0x80) c|= 0xffffff00;
    int b = (instr)     & 31;

    int n13 = (c<<5) | b;
    int n13d = (c<<5) | d;
    int n21 = (c<<13) | (i<<10) | (a<<5) | b;

    switch(k) {
        case KIND_ALU_REG: setReg(d, aluOp(i, regs[a], regs[b], c));   break;
        case KIND_ALU_LIT: setReg(d, aluOp(i, regs[a], n13, c));       break;
        case KIND_LOAD:    setReg(d, read(regs[a]+n13, i));            break;
        case KIND_STORE:   write(regs[a]+n13d, i, regs[b]);            break;
        case KIND_BRANCH:  if (comp(i, regs[a], regs[b])) {
                                pc = pc + 4* n13d;
                                if (debug>=2)
                                    fprintf(debug_trace,"JMP %08x", pc);
                            } else {
                                if (debug>=2)
                                    fprintf(debug_trace,"BRANCH NOT TAKEN");
                            }
                            
                           break;
        case KIND_JUMP:     setReg(d,pc); pc = pc + 4* n21;             
                           if (debug>=2) {
                                if (d!=0) fprintf(debug_trace,"  ");
                            fprintf(debug_trace,"JMP %08x", pc);
                           }
                           break;
        case KIND_JUMP_REG:setReg(d,pc); pc = regs[a] + n13;           
                           if (debug>=2)
                             fprintf(debug_trace,"JMP %08x", pc);
                            break;
        case KIND_LD_LIT:  setReg(d, n21 << 11);                       break;
        case KIND_ADD_PC:  setReg(d, pc + n21*4);                      break;
        case KIND_CFG:     cfg_cmd(i, d, n13, regs[a]);                break;
        case KIND_MUL:     setReg(d, mult_op(i, regs[a], regs[b]));    break;
        case KIND_MUL_LIT: setReg(d, mult_op(i, regs[a], n13));        break;
        default:           exception(EXCEPTION_ILLEGAL_INSTRUCTION, instr); break;
    }

    if(debug>=2)
        fprintf(debug_trace,"\n");
}


static void output_final_regs(FILE *out) {
    fprintf(out,"\nProgram completed\n");
    for(int reg=1; reg<=31; reg++) {
        fprintf(out,"$%-2d=%08X ",reg, regs[reg]);
        if (reg%6==0)
            fprintf(out, "\n");
    }
    fprintf(out, "\n");
}

static void execute() {
    //int timeout = 100000000;
    int timeout = 100000;

    while(!finished) {
        int ins = read(pc,2);
        pc += 4;
        executeInstruction(ins);
        if (pc==0) 
            finished = 1;
        if (timeout-- == 0) {
            printf("Timeout\n");
            finished = 1;
        }
    }

    if (debug>=2) output_final_regs(debug_trace);
}

int main(int argc, string* argv)
{
    ram = calloc(RAM_SIZE,1);
    regs[31] = RAM_SIZE;

    if (ram==0)
        fatal("Out of memory");

    string filename = 0;

    for(int index=1; index<argc; index++) {
        string arg = argv[index];
        if (!strcmp(arg,"-r"))
            debug = 1;
        else if (!strcmp(arg,"-d"))
            debug = 2;
        else {
            if (filename!=0)
                fatal("Unrecognised argument '%s",arg);
            filename = arg;
        }
    }

    if (debug)
        debug_trace = fopen("debug.log","w");
    reg_trace = fopen("sim_regs.log","w");
    uart_out  = fopen("sim_uart.log","w");

    if (filename==0)
        fatal("No filename specified");

    read_file(filename);
    // if (debug)
    //     read_labels();

    pc = 0xFFFF0000;
    execute();
}

