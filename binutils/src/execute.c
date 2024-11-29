#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include "f32.h"

static int reg[32];  // The CPU registers
static unsigned int pc;       // The program counter

extern int* prog_mem;  // program memory
extern int* data_mem;  // data memory

static FILE* reg_log;  // log register values to this file
static FILE* uart_log;
extern FILE* trace_file;    

// ================================================
//                  set_reg
// ================================================

static void set_reg(int reg_num, int value) {
    if (reg_num==0)
        return;
    reg[reg_num] = value;
    if (reg_log)
        fprintf(reg_log, "$%2d = %08x\n", reg_num, value);
    if (trace_file)
        fprintf(trace_file, "$%2d = %08x", reg_num, value);
}

// ================================================
//                  alu operation
// ================================================

static int alu_op(int op, int a, int b, int c) {
    switch(op) {
        case 0: return a & b;
        case 1: return a | b;
        case 2: return a ^ b;
        case 3: switch(c&3) {
            case 0: return a << (b & 31);
            case 1: return 0;    
            case 2: return ((unsigned)a) >> (b & 31);
            case 3: return ((signed)a) >> (b & 31);
        }
        case 4: return a + b;
        case 5: return a - b;
        case 6: return a < b;
        case 7: return ((unsigned)a) > ((unsigned)b);
        default: return 0;
    }
}

// ================================================
//                  mul operation
// ================================================

static int mul_op(int op, int a, int b) {
    switch(op) {
        case 0: return a * b;
        case 4: return (b==0) ? -1 : ((unsigned)a) / ((unsigned)b);
        case 5: return (b==0) ? -1 : ((signed)a) / ((signed)b);
        case 6: return (b==0) ? a  : ((unsigned)a) % ((unsigned)b);
        case 7: return (b==0) ? a  : ((signed)a) % ((signed)b);
        default: return 0;
    }
}

// ================================================
//                  branch operation
// ================================================

static int branch_op(int op, int a, int b) {
    switch(op) {
        case 0: return a == b;
        case 1: return a != b;
        case 2: return a < b;
        case 3: return a >= b;
        case 4: return ((unsigned)a) < ((unsigned)b);
        case 5: return ((unsigned)a) >= ((unsigned)b);
        default: return 1;
    }
}



// ================================================
//                  write_hwregs
// ================================================

static void write_hwregs(unsigned int addr, int value) {
    switch(addr) {
        case 0xE0000000:
            // fprintf(uart_log, "7-Segment = %06x\n", value&0xffffff);
            // printf("7-Segment = %06x\n", value&0xffffff);
            break;

        case 0xE0000004:
            // fprintf(uart_log, "LEDs = %x\n", value&0x3ff);
            // printf("LEDs = %x\n", 0x3ff);
            break;

        case 0xE0000010:
            fprintf(uart_log, "%c", value);
            printf("%c", value);
            break;

        default:
            printf("write_hwregs(%08x, %08x)\n", addr, value);
        break;
    }
}

// ================================================
//                  read_hwregs
// ================================================

static int read_hwregs(unsigned int addr) {
    switch(addr) {
        case 0xE0000010:   // UART TX
            return 0x3ff;  // Report the space in the fifo - fake it to always be empty

        case 0xE0000030:   // Simulation flag
            return 1;      // Returns 1 in simulations, zero on real hardware

        default:
            printf("read_hwregs(%08x)\n", addr);
            return 0xdeadbeef;

        break;
    }

}

// ================================================
//                  read_memory
// ================================================

static int read_memory(unsigned int addr) {
    if (addr < 0x4000000)
        return data_mem[addr>>2];
    else if (addr>=0xE0000000 && addr<0xE0001000)
        return read_hwregs(addr);
    else if (addr>=0xffff0000)
        return prog_mem[(addr & 0xffff)>>2];
    else
        return 0xBAADF00D;
}


// ================================================
//                  write_memory
// ================================================

static void write_memory(unsigned int addr, int value, int mask) {
    if (addr < 0x4000000) {
        int a = addr >> 2;
        data_mem[a] = (data_mem[a] & ~mask) | (value & mask);
        if (trace_file)
            fprintf(trace_file, "[%08x] = %08x", addr, data_mem[a]);
    } else if (addr>=0xE0000000 && addr<0xE0001000) {
        write_hwregs(addr, value);
        if (trace_file)
            fprintf(trace_file, "[%08x] = %08x", addr, value);
    } else if (addr>=0xffff0000) {
        int a = (addr & 0xffff) >> 2;
        prog_mem[a] = (prog_mem[a] & ~mask) | (value & mask);
    }
}

// ================================================
//                  write_memory_size
// ================================================

static void write_memory_size(unsigned int addr, int value, int size) {
    int mask = 0;
    int shift = (addr & 3) * 8;
    switch(size) {
        case 0: 
            // Byte
            mask = 0xff << shift; 
            value = (value & 0xff) << shift;
            break;
        case 1: 
            // Halfword
            if (addr & 2)
                fatal("write_memory_size: halfword misaligned");
            mask = 0xffff << shift;
            value = (value & 0xffff) << shift;
            break;

        case 2: 
            // Word
            if (addr & 3)
                fatal("write_memory_size: word misaligned");
            mask = 0xffffffff; 
            break;

        default:
            fatal("write_memory_size: invalid size %d", size);
    }
    write_memory(addr, value, mask);
}

// ================================================
//                  read_memory_size
// ================================================

static int read_memory_size(unsigned int addr, int size) {
    int value = read_memory(addr& 0xfffffffc);;
    int shift = (addr & 3) * 8;
    switch(size) {
        case 0:
            // Byte
            value = (value >> shift) & 0xff;
            if (value & 0x80)
                value = value | 0xffffff00;
            break;
        case 1:
            // Halfword
            if (addr & 2)
                fatal("read_memory_size: halfword misaligned");
            value = (value >> shift) & 0xffff;
            if (value & 0x8000)
                value = value | 0xffff0000;
            break;

        case 2:
            // Word
            if (addr & 3)
                fatal("read_memory_size: word misaligned");
            break;

        default:
            fatal("read_memory_size: invalid size %d", size);
    }
    return value;
}


// ================================================
//                  execute_instruction
// ================================================

static void execute_instruction(int instr) {
    int k = (instr >> 26) & 0x3f;
    int i =  (instr >> 23) & 0x7;
    int d = (instr >> 18) & 0x1f;
    int a = (instr >> 13) & 0x1f;
    int c = (instr >> 5) & 0xff; 
    if (c&0x80)
        c = c | 0xffffff00; // sign extend
    int b = (instr >> 0) & 0x1f;

    int n13  = (c<<5) | b;
    int n13s = (c<<5) | d;
    int n21 = (c<<13) | (i<<10) | (a<<5) | b;

    switch (k) {
        case KIND_ALU:  set_reg(d, alu_op(i, reg[a], reg[b], c));  break;
        case KIND_ALUI: set_reg(d, alu_op(i, reg[a], n13, c)); break;
        case KIND_BRA:  if (branch_op(i, reg[a], reg[b])) {
                            pc = pc + n13s * 4;
                            if (trace_file)
                                fprintf(trace_file, "-> %s", find_label(pc));
                        }
                        break;
        case KIND_LD:   set_reg(d, read_memory_size(reg[a] + n13, i)); break;
        case KIND_ST:   write_memory_size(reg[a] + n13s, reg[b], i); break;
        case KIND_JMP:  set_reg(d,pc); 
                        pc += n21*4; 
                        if (trace_file)
                            fprintf(trace_file, "-> %s", find_label(pc));
                        break;
        case KIND_JMPR: set_reg(d,pc);
                        pc = reg[a] + 4*n13; 
                        if (trace_file)
                            fprintf(trace_file, "-> %s", find_label(pc));                        
                        break;
        case KIND_LDU:  set_reg(d, n21<<11); break;
        case KIND_LDPC: set_reg(d, pc + n21*4); break;
        case KIND_MUL:  set_reg(d, mul_op(i, reg[a],  reg[b])); break;
        case KIND_MULI: set_reg(d, mul_op(i, reg[a],  n13)); break;
        default: break;
    }
}

// ================================================
//                  execute
// ================================================

void execute() {
    pc = 0xffff0000;
    int timeout = 100000;
    reg_log = fopen("sim_reg.log", "w");
    uart_log = fopen("sim_uart.log", "wb");

    while (timeout>0 && pc!=0) {
        int instr = read_memory(pc);
        if (trace_file) 
            fprintf(trace_file, "%08x: %-40s", pc, disassemble_line(instr,pc+4));
        pc += 4;
        execute_instruction(instr);
        if (trace_file) 
            fprintf(trace_file, "\n");
        timeout--;    
    }
    if (timeout==0)
        printf("Timeout\n");
}
