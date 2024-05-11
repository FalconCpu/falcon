#include <stdio.h>
#include "f32asm.h"
#include "f32_defs.h"

static int debug_level = 0;
// Log:
//     0 = NOTHING
//     1 = MEMORY WRITES
//     2 = REGISTER WRITES

static int regs[32];
#define MEM_SIZE 0x40000
static int chip_ram[MEM_SIZE];
#define PROG_SIZE 0x10000
extern int prog[PROG_SIZE];
static int pc = 0xffff0000;

#define EXCEPTION 0


static void set_reg(int reg, int value) {
    if (reg!=0) {
        regs[reg]=value;
        if (debug_level>=2)
            printf("$%d=%d",reg,value);
    }
}

static int alu_op(int op, int a, int b, int c) {
    switch(op) {
        case 0: return a & b;
        case 1: return a | b;
        case 2: return a ^ b;
        case 3: if (c==0)
                    return a << b;
                else if (c==-1)
                    return a >> b;
                else if (c==-3)
                    return ((unsigned int)a) >> b;
                else
                    return EXCEPTION;
        case 4: return a + b;
        case 5: return a - b;
        case 6: return a < b;
        default:return (unsigned)a < (unsigned)b;
    }
}

static int sign_extend_halfword(int val) {
    if (val & 0x8000)
        return val | 0xffff0000;
    else
        return val;
}

static int sign_extend_byte(int val) {
    if (val & 0x80)
        return val | 0xffffff00;
    else
        return val;
}

static int read_mem(unsigned int addr) {
    if (addr>=0 && addr<=MEM_SIZE)
        return chip_ram[addr/4];
    else if (addr>=0xffff0000)
        return prog[(addr&0xfffc)/4];
    else
        return EXCEPTION;
}

static void write_mem(unsigned int addr, int value, int mask) {
    if (addr>=0 && addr<=MEM_SIZE)
        chip_ram[addr/4] = (chip_ram[addr/4] & ~mask) | (value & mask);
    else if (addr>=0xffff0000)
        ; // do nothing
    else
        return ;
}

static int read_op(unsigned int addr, int op) {
    int lsb = 8*(addr & 3);
    int msb = addr & 0xfffffffc;

    switch(op) {
        case 0:
            return read_mem(msb) >> lsb;
        case 1:
            if (lsb & 8)
                return EXCEPTION;
            else
                return read_mem(msb) >> lsb;
        case 2:
            if (lsb!=0)
                return EXCEPTION;
            else
                return read_mem(msb);
        default:
            return EXCEPTION;
    }

}

static void write_op(unsigned int addr, int op, int value) {
    int lsb = 8*(addr & 3);
    int msb = addr & 0xfffffffc;

    switch(op) {
        case 0:
            write_mem(msb, value<<lsb, 0xff << lsb);
            return;
        case 1:
            if (lsb & 8)
                return ;
            write_mem(msb, value<<lsb, 0xffff << lsb);
            return;
        case 2:
            if (lsb!=0)
                return;
            write_mem(msb, value, -1);
            return;
        default:
            return;
    }
}

static void execute() {
    int instr = prog[pc/4];
    int k = (instr>>26) & 31;
    int i = (instr>>23) & 7;
    int d = (instr>>18) & 31;
    int a = (instr>>13) & 31;
    int c = (instr>>5) & 0xff;
    if (c>=128)
        c |= 0xffffff80;
    int b = instr & 31;
    int s13 = (c<<5) | b;
    int s13d = (c<<5) | d;
    int s21 = (c<<13) | (i<<10) | (a<<5) | b;

    switch(k) {
        case KIND_ALU_REG: set_reg(d, alu_op(i, reg[a], reg[b], c);
                           break;
        case KIND_ALU_LIT: set_reg(d, alu_op(i, reg[a], s13, c);
                           break;
        case KIND_LOAD:    set_reg(d, read_op())
            sprintf(buf,"%-4s %s, %s[%d]", load_name[i], reg_name[d], reg_name[a], s13);  break;
        case KIND_STORE:   sprintf(buf,"%-4s %s, %s[%d]", store_name[i],reg_name[b], reg_name[a], s13d);  break;
        case KIND_BRANCH:  sprintf(buf,"%-4s %s, %s, %08XH", branch_name[i],reg_name[a], reg_name[b], addr + 4 + 4*s13d);  break;
        case KIND_JUMP:    sprintf(buf,"jmp  %s, %s",   reg_name[d], display_address(addr + 4 + 4*s21));  break;
        case KIND_JUMP_REG:sprintf(buf,"jmp  %s, %s[%d]", reg_name[d], reg_name[a], s13);  break;
        case KIND_LD_LIT:  sprintf(buf,"ld   %s, %d",  reg_name[d], s21<<11); break;
        case KIND_ADD_PC:  sprintf(buf,"ldpc %s , %s",reg_name[d], display_address(addr + 4 + 4*s21)); break;
        default:           sprintf(buf,"reserved");
    }

}

