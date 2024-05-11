#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "f32asm.h"
#include "f32_defs.h"

#define MEM_SIZE 0x40000
extern int prog[MEM_SIZE/4];
extern int prog_length;
extern token* all_labels;

static string reg_name[] = {
    "0"  ,"$1","$2","$3","$4","$5","$6","$7","$8","$9",
    "$10","$11","$12","$13","$14","$15","$16","$17","$18","$19",
    "$20","$21","$22","$23","$24","$25","$26","$27","$28","$29",
    "$30","$sp"
};

static string alu_name[] = {
    "and","or","xor","shift","add","sub","clt","cltu"
};

static string load_name[] = {
    "ldb","ldh","ldw"
};

static string store_name[] = {
    "stb","sth","stw"
};

static string branch_name[] = {
    "beq","bne","blt","bge","bltu","bgeu"
};

static string mul_name[] = {
    "mul","","","","div","mod"
};

static string cfg_op[] = {
    "cfgr","cfgw", "cjmp"
};

static string sysregs[] = {
    "ecause","epc","edata","escratch"
};


/// --------------------------------------
///               display_address
/// --------------------------------------

static string display_address(int address) {
    for(token *t=all_labels; t; t=t->next)
        if (t->value==address && (t->flags & FLAG_DEFINED))
            return t->name;

    static char buf[16];
    sprintf(buf,"0%08XH", address);
    if (isdigit(buf[1]))
        return buf+1;
    else
        return buf;
}

/// --------------------------------------
///              alu_op_name
/// --------------------------------------

static string alu_op_name(int op, int c) {
    if (op==3) {
        switch(c) {
            case 0:  return "lsl";
            case -1: return "lsr";
            case -3: return "asr";
            default: return "reserved";
        }

    } else 
        return alu_name[op];
}




/// --------------------------------------
///               disassemble_line
/// --------------------------------------

char* disassemble_line(int addr) {
    static char buf[64];

    int instr = prog[(addr & 0xffff)/4];
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
        case KIND_ALU_REG:  if (i==1 && b==0)
                                sprintf(buf,"ld   %s, %s", reg_name[d], reg_name[a]);
                            else
                                sprintf(buf,"%-4s %s, %s, %s", alu_op_name(i,c), reg_name[d], reg_name[a], reg_name[b]);
                            break;
        case KIND_ALU_LIT:  if (i==1 && a==0)
                                sprintf(buf,"ld   %s, %d", reg_name[d], s13);
                            else
                                sprintf(buf,"%-4s %s, %s, %d", alu_op_name(i,c), reg_name[d], reg_name[a], s13);
                            break;
        case KIND_LOAD:     sprintf(buf,"%-4s %s, %s[%d]", load_name[i], reg_name[d], reg_name[a], s13);  break;
        case KIND_STORE:    sprintf(buf,"%-4s %s, %s[%d]", store_name[i],reg_name[b], reg_name[a], s13d);  break;
        case KIND_BRANCH:   sprintf(buf,"%-4s %s, %s, %s", branch_name[i],reg_name[a], reg_name[b], display_address(addr + 4 + 4*s13d));  break;
        case KIND_JUMP:     sprintf(buf,"jmp  %s, %s",   reg_name[d], display_address(addr + 4 + 4*s21));  break;
        case KIND_JUMP_REG: if (a==30 && d==0 && s13==0)
                                sprintf(buf,"ret");
                            else
                                sprintf(buf,"jmp  %s, %s[%d]", reg_name[d], reg_name[a], s13);  
                            break;
        case KIND_LD_LIT:   sprintf(buf,"ld   %s, %d",  reg_name[d], s21<<11); break;
        case KIND_ADD_PC:   sprintf(buf,"ldpc %s , %s",reg_name[d], display_address(addr + 4 + 4*s21)); break;
        case KIND_MUL:      sprintf(buf,"%-4s %s, %s, %s", mul_name[i], reg_name[d], reg_name[a], reg_name[b]); break;
        case KIND_CFG:      if (s13<0 || s13>4)
                                sprintf(buf,"invalid sysreg");
                            else if (i==2)
                                sprintf(buf,"jmp %s", sysregs[s13]);
                            else 
                                sprintf(buf,"%-4s %s, %s, %s", cfg_op[i], reg_name[d], reg_name[a], sysregs[s13]); 
                            break;
        case KIND_SYS:      sprintf(buf,"sys  %d", s13); break;
        default:            sprintf(buf,"reserved");
    }
    return buf;
}

static void print_label(int pc) {
    for(token*t = all_labels; t; t=t->next)
        if (t->value==pc && (t->flags&FLAG_DEFINED))
            printf("%04X           %s:\n",pc, t->name);
}


void disassemble() {
    for(int pc=0; pc<prog_length; pc+=4) {
        print_label(pc);
        printf("%04X %08X  %s\n",pc,prog[pc/4],disassemble_line(pc));
    }
}


void read_labels() {
    FILE* fh = fopen("labels.txt","r");
    if (fh==0)
        return;

    char buf[256];
    while(!feof(fh)) {
        int val;

        fscanf(fh,"%x ", &val);
        buf[0]=0;
        buf[1]=0;
        fgets(buf, 256, fh);
        if (buf[0]>' ') {
            buf[strlen(buf)-1] = 0; // remove end of line

            token* label = new(token);
            label->value = val;
            label->name = strdup(buf);
            label->next = all_labels;
            label->flags = FLAG_DEFINED;
            all_labels = label;
        }
    }   
    fclose(fh);
}

void read_file(string file_name) {
    char buf[256];
    FILE* fh = fopen(file_name,"r");
    if (fh==0)
        fatal("Cannot open file '%s'", file_name);
    prog_length = 0x8000;   // account for the start position where the code gets loaded

    while(!feof(fh)) {
        buf[0]=0;
        fgets(buf,256,fh);
        int inst;
        int count = sscanf(buf,"%x",&inst);
        if (count==1) {
            prog[prog_length/4] = inst;
            prog_length += 4;
        }
    }
    fclose(fh);

    read_labels();
}
