#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "f32.h"

int org = 0xffff0000;

Token all_labels;

// ================================================
//                 text values
// ================================================

static string reg_name[] = {
    "0",
    "$1",
    "$2",
    "$3",
    "$4",
    "$5",
    "$6",
    "$7",
    "$8",
    "$9",
    "$10",
    "$11",
    "$12",
    "$13",
    "$14",
    "$15",
    "$16",
    "$17",
    "$18",
    "$19",
    "$20",
    "$21",
    "$22",
    "$23",
    "$24",
    "$25",
    "$26",
    "$27",
    "$28",
    "$29",
    "$30",
    "$sp"
};

string alu_names[] = {
    "and",
    "or",
    "xor",
    "",
    "add",
    "sub",
    "clt",
    "cltu",
};

string bra_names[] = {
    "beq",
    "bne",
    "blt",
    "bge",
    "bltu",
    "bgeu",
    "",
    "",
};

string load_names[] = {
    "ldb",
    "ldh",
    "ldw",
    "",
    "",
    "",
    "",
    "",
};

string store_names[] = {
    "stb",
    "sth",
    "stw",
    "",
    "",
    "",
    "",
    "",
};

string shift_names[] = {
    "lsl",
    "",
    "lsr",
    "asr",
    "",
    "",
    "",
    "",
};

string mul_names[] = {
    "mul",
    "",
    "",
    "",
    "divu",
    "divs",
    "modu",
    "mods",
};

string cfg_name[] = {
    "!version",
    "!epc",   
    "!ecause",
    "!edata", 
    "!estatus",
    "!escratch",
    "!status"
};


// ================================================
//                 alu_names
// ================================================

static string decode_alu_name(int op, int c) {
    if (op==3)
        return shift_names[c&3];
    else
        return alu_names[op];
}

// ================================================
//                 find_label
// ================================================

string find_label(int addr) {
    for(Token t = all_labels; t; t=t->next)
        if (t->value == addr)
            return t->text;

    static char buf[16];
    sprintf(buf, "0x%08x", addr);
    return buf;
}


// ================================================
//                 disassemble_line
// ================================================

char *disassemble_line(int op, int pc) {
    static char line[100];

    int k = (op >> 26) & 0x3f;
    int i =  (op >> 23) & 0x7;
    int d = (op >> 18) & 0x1f;
    int a = (op >> 13) & 0x1f;
    int c = (op >> 5) & 0xff; 
    if (c&0x80)
        c = c | 0xffffff00;
    int b = (op >> 0) & 0x1f;

    int n13  = (c<<5) | b;
    int n13s = (c<<5) | d;
    int n21 = (c<<13) | (i<<10) | (a<<5) | b;

    switch(k) {
        case 0x10: 
            if (i==1 && b==0)
                sprintf(line, "ld %s, %s", reg_name[d], reg_name[a]);
            else
                sprintf(line, "%s %s, %s, %s", decode_alu_name(i,c), reg_name[d], reg_name[a], reg_name[b]); 
            break;
        case 0x11: 
            if (i==1 && a==0)
                sprintf(line, "ld %s, %d", reg_name[d], n13);
            else
                sprintf(line, "%s %s, %s, 0x%x", decode_alu_name(i,c), reg_name[d], reg_name[a], n13); break;
        case 0x12: sprintf(line, "%s %s, %s[%d]", load_names[i], reg_name[d], reg_name[a], n13); break;
        case 0x13: sprintf(line, "%s %s, %s[%d]", store_names[i], reg_name[b], reg_name[a], n13s); break;
        case 0x14: sprintf(line, "%s %s, %s, %s", bra_names[i], reg_name[a], reg_name[b], find_label(pc+4*n13s)); break;
        case 0x15: 
            if (d==0)
                sprintf(line, "jmp %s", find_label(pc+4*n21));
            else if (d==30)
                sprintf(line, "jsr %s", find_label(pc+4*n21));
            else
                sprintf(line, "jmp %s, %s", reg_name[d], find_label(pc+4*n21)); break;
            break;
        case 0x16: 
            if (a==30 && n13==0)
                sprintf(line, "ret");
            else
            sprintf(line, "jmp %s, %s[%d]", reg_name[d], reg_name[a], n13); 
            break;
        case 0x17: sprintf(line, "ld %s, 0x%x", reg_name[d], n21<<11); break;
        case 0x18: sprintf(line, "ldpc %s, %s", reg_name[d], find_label(pc+4*n21)); break;
        case 0x19: sprintf(line, "%s %s, %s, %s", mul_names[i], reg_name[d], reg_name[a], reg_name[b]); break;
        case 0x1a: sprintf(line, "%s %s, %s, 0x%x", mul_names[i], reg_name[d], reg_name[a], n13); break;
        case 0x1b:
            if (i==0)
                sprintf(line, "cfg %s,%s", reg_name[d], cfg_name[b]);
            else if (i==1)
                sprintf(line, "cfg %s, %s, %s", reg_name[d], cfg_name[b], reg_name[a]);
            else if (i==2 && a==0 && b==0 && c==0)
                sprintf(line, "rte");
            else
                sprintf(line, "undefined cfg %d",i);
                break;
        default: sprintf(line, "undefined"); break;
    }
    return line;
}

// ================================================
//                 disassemble_program
// ================================================

void disassemble_program(int *program, int len) {
    int pc = 0;
    while (pc < len*4) {
        for(Token t = all_labels; t; t=t->next)
            if (t->value == org + pc)
                printf("%08x:          %s:\n", t->value, t->text);

        int i = program[pc/4];
        printf("%08x: %08x %s\n", org+pc, i, disassemble_line(i, org + pc+4));
        pc += 4;
    }
}


// ================================================
//                 load_labels
// ================================================

void load_labels(string filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL)
        return;

    char line[100];
    while (fgets(line, sizeof(line), file) != NULL) {
        if (line[strlen(line)-1]=='\n')
            line[strlen(line)-1] = 0;
        char *text;
        int label_address = strtoul(line,&text,16);
        if (*text==' ')
            text++;
        Token token = my_malloc(sizeof(struct Token));
        token->text = _strdup(text);
        token->value = label_address;
        token->next = all_labels;
        all_labels = token;
    }

    fclose(file);
}