#include <stdio.h>
#include <string.h>
#include "f32asm.h"
#include "f32_defs.h"

#define MEM_SIZE 0x40000
int prog[MEM_SIZE/4];
int prog_length;

static int latent_reg = 0;

extern int line_number;
extern string file_name;
extern token* all_strings;
extern token* all_labels;
reference* all_references = 0;
string current_block;

static int origin = 0xffff0000;

/// -----------------------------------------------------------
///                      add_instr
/// -----------------------------------------------------------

static void add_instr(int i) {
    if (prog_length==MEM_SIZE)
        fatal("Out of prgram memory");
    prog[prog_length/4] = i;
    prog_length+=4;
}

/// -----------------------------------------------------------
///                      format
/// -----------------------------------------------------------

static int format_r(int kind, int op, int d, int a, int b, int c) {
    assert(kind>=0 && kind<=0x3f);
    assert(op>=0 && op<=7);
    assert(d>=0 && d<=31);
    assert(a>=0 && a<=31);
    assert(b>=0 && b<=31);
    assert(c>=-128 && c<=127);

    // if (a==latent_reg && latent_reg!=0)
    //     error("Use of register $%d in delay slot", a);
    // if (b==latent_reg && latent_reg!=0)
    //     error("Use of register $%d in delay slot", b);
    latent_reg = 0;

    return( (kind<<26) | (op<<23) | (d<<18) | (a<<13) | ((c&0xff)<<5) | b);
}

static int format_i(int kind, int op, int d, int a, int b) {
    assert(kind>=0 && kind<=0x3f);
    assert(op>=0 && op<=7);
    assert(d>=0 && d<=31);
    assert(a>=0 && a<=31);
    if (b<-0x1000 || b>=0x1000)
        error("Integer %d out of range -4096..4095", b);

    // if (a==latent_reg && latent_reg!=0)
    //     error("Use of register $%d in delay slot", a);
    latent_reg = 0;

    return( (kind<<26) | (op<<23) | (d<<18) | (a<<13) | (b&0x1fff));
}

static int format_s(int kind, int op, int a, int b, int c) {
    assert(kind>=0 && kind<=0x3f);
    assert(op>=0 && op<=7);
    assert(a>=0 && a<=31);
    assert(b>=0 && b<=31);
    if (c<-0x1000 || c>=0x1000)
        error("Integer %d out of range -4096..4095", c);
    // if (a==latent_reg && latent_reg!=0)
    //     error("Use of register $%d in delay slot", a);
    // if (b==latent_reg && latent_reg!=0)
    //     error("Use of register $%d in delay slot", b);
    latent_reg = 0;


    return( (kind<<26) | (op<<23) | ((c&31)<<18) | (a<<13) | (c&0x1fe0) | b);
}

static int format_j(int kind, int d, int lit) {
    assert(kind>=0 && kind<=0x3f);
    assert(d>=0 && d<=31);
    if (lit<-0x100000 || lit>=0x100000)
        error("Integer %d out of range -4096..4095", lit);

    int b= lit & 31;
    int a = (lit>>5) & 31;
    int op = (lit>>10) & 7;
    int c = (lit>>13) & 0xff;
    latent_reg =0;

    return( (kind<<26) | (op<<23) | (d<<18) | (a<<13) | (c<<5) | b);
}

/// -----------------------------------------------------------
///                      add_reference
/// -----------------------------------------------------------

static int add_reference(token *tok) {
    if (tok->flags& FLAG_DEFINED) {
        return (tok->value - (prog_length+4))/4;
    } else {
        reference *ref = new(struct reference);
        ref->token = tok;
        ref->location = prog_length;
        ref->file_name = file_name;
        ref->line_number = line_number;
        ref->next = all_references;
        all_references = ref;
        return 0;
    }
}


/// -----------------------------------------------------------
///                      resolve_references
/// -----------------------------------------------------------

static void resolve_references() {
    latent_reg = 0;

    for(reference *ref = all_references; ref; ref=ref->next) {
        if (!(ref->token->flags & FLAG_DEFINED)){
            file_name = ref->file_name;
            line_number = ref->line_number;
            error("Undefined label '%s'",ref->token->name);
        } else {
            int instr = prog[ref->location/4];
            int k = (instr>>26) & 31;
            int i = (instr>>23) & 7;
            int d = (instr>>18) & 31;
            int a = (instr>>13) & 31;
            int b = (instr>>0) & 31;
            switch(k) {
                case KIND_BRANCH: prog[ref->location/4] = format_s(k,i,a,b, (ref->token->value - ref->location -4)/4); break;
                case KIND_JUMP:
                case KIND_ADD_PC: prog[ref->location/4] = format_j(k,d,(ref->token->value - ref->location -4)/4); break;
                case 0:           prog[ref->location/4] = ref->token->value + origin; break;
                default:          fatal("Got kind %x in resolve_references",k);
            }
        }
    }
}


/// -----------------------------------------------------------
///                      match_format
/// -----------------------------------------------------------

static int match_char(int format, int line) {
    return format==line ||
           (format=='$' && line=='0') ||
           (format=='i' && line=='0');
}

static int match_format(string format, token** line) {
    int k = 0;
    while(format[k] && line[k]) {
        if (!match_char(format[k], line[k]->kind))
            return 0;
        k++;
    }

    return format[k]==0 && line[k]==0;
}


/// -----------------------------------------------------------
///                      assemble_line
/// -----------------------------------------------------------

static char* get_line_format(token** line) {
    static char buf[16];
    int k = 0;
    while(line[k]) {
        buf[k] = line[k]->kind;
        k++;
    }

    buf[k] = 0;
    return buf;
}

static void define_label(token *label) {
    if (label->flags & FLAG_DEFINED)
        error("Multiple definitions for label '%s'", label->name);
    label->value = prog_length;
    label->flags |= FLAG_DEFINED;

    // if the label does not contain a '@' then it becomes the current block
    for(const char*t = label->name; *t; t++)
        if (*t=='@')
            return;
    current_block = label->name;
}

static void add_load_int(int reg, int value) {
    if (value>=-0x1000 && value<0x1000)
        add_instr(format_i(KIND_ALU_LIT, 1, reg, 0, value));
    else {
        add_instr(format_j(KIND_LD_LIT, reg, value>>11));
        int residue = value & 0x7ff;
        if (residue!=0)
            add_instr(format_i(KIND_ALU_LIT, 1, reg, reg, residue));
    }
}

static void process_dcb(token** line) {
    int index;
    int n = 0;
    int x = 0;

    for(index=1; line[index]; index++) {
        switch(line[index]->kind) {
            case '0':
            case 'i': {
                int v = line[index]->value;
                if (v<-128 || v>255) {
                    printf("%c %s %x\n", line[index]->kind, line[index]->name, line[index]->value);
                    error("Value %d out of range -128..127", v);
                }
                x |= (v&0xff) << (n*8);
                n++;
                if (n==4) {
                    add_instr(x);
                    n=0;
                    x=0;
                }
                break;
            }

            case 'l':
            case '"':
                printf("%c %s\n", line[index]->kind, line[index]->name);
                error("labels only allowed in dcw");
                break;

            case ',':
                break;

            default:
                error("Got '%s' when expecting contant", line[index]->name);
        }
    }

    if (n)
        add_instr(x);
}

static void process_dch(token** line) {
    int index;
    int n = 0;
    int x = 0;

    for(index=1; line[index]; index++) {
        switch(line[index]->kind) {
            case '0':
            case 'i': {
                int v = line[index]->value;
                if (v<-32768 || v>32767)
                    error("Value %d out of range -8000H..7FFFH");
                x |= (v&0xffff) << (n*16);
                n++;
                if (n==2) {
                    add_instr(x);
                    n=0;
                    x=0;
                }
                break;
            }

            case 'l':
            case '"':
                error("labels only allowed in dcw");
                break;

            case ',':
                break;

            default:
                error("Got '%s' when expecting contant", line[index]->name);
        }
    }

    if (n)
        add_instr(x);

}

static void process_dcw(token** line) {
    int index;

    for(index=1; line[index]; index++) {
        switch(line[index]->kind) {
            case '0':
            case 'i': {
                int v = line[index]->value;
                add_instr(v);
                break;
            }

            case 'l':
            case '"':
                add_instr(add_reference(line[index]));
                break;

            case ',':
                break;

            default:
                error("Got '%s' when expecting contant", line[index]->name);
        }
    }
}



static void process_dc(token** line) {
    int size = line[0]->value;
    switch (size)
    {
        case 0:  process_dcb(line); break;
        case 1:  process_dch(line); break;
        default: process_dcw(line); break;
    }
}


static void define_constant(token *label, int value) {
    if (label->flags & FLAG_DEFINED)
        error("Multiple definitions for label '%s'", label->name);
    else {
        label->kind = 'i';
        label->value = value;
    }
}

static void assemble_line(token** line) {

    #define match(FORMAT) if (match_format(FORMAT,line))
    #define V0 line[0]->value
    #define V1 line[1]->value
    #define V2 line[2]->value
    #define V3 line[3]->value
    #define V4 line[4]->value
    #define V5 line[5]->value

    if (line[0] && line[1] && line[0]->kind=='l' && line[1]->kind==':') {
        define_label(line[0]);
        line += 2;
    }

    if        (line[0]==0)  ;
    else match("A$,$,$")    add_instr(format_r(KIND_ALU_REG, V0, V1, V3, V5, 0));
    else match("A$,$")      add_instr(format_r(KIND_ALU_REG, V0, V1, V1, V3, 0));
    else match("A$,$,i")    add_instr(format_i(KIND_ALU_LIT, V0, V1, V3, V5));
    else match("A$,i")      add_instr(format_i(KIND_ALU_LIT, V0, V1, V1, V3));
    else match("L$,$[i]")   add_instr(format_i(KIND_LOAD,    V0, V1, V3, V5));
    else match("S$,$[i]")   add_instr(format_s(KIND_STORE,   V0, V3, V1, V5));
    else match("D$,$[i]")   add_instr(format_i(KIND_LOAD,    2, V1, V3, V5));
    else match("D$[i],$")   add_instr(format_s(KIND_STORE,   2, V1, V5, V3));
    else match("B$,$,l")    add_instr(format_s(KIND_BRANCH,  V0, V1, V3, add_reference(line[5])));
    else match("D$,$")      add_instr(format_r(KIND_ALU_REG,  1, V1, V3, 0, 0));
    else match("D$,i")      add_load_int(V1, V3);
    else match("D$,l")      add_instr(format_j(KIND_ADD_PC, V1, add_reference(line[3])));
    else match("D$,\"")     add_instr(format_j(KIND_ADD_PC, V1, add_reference(line[3])));
    else match("H$,$,$")    add_instr(format_r(KIND_ALU_REG, 3, V1, V3, V5, V0));
    else match("H$,$,i")    add_instr(format_r(KIND_ALU_LIT, 3, V1, V3, V5, V0));
    else match("H$,i")      add_instr(format_r(KIND_ALU_LIT, 3, V1, V1, V3, V0));
    else match("Jl")        add_instr(format_j(KIND_JUMP,  0, add_reference(line[1])));
    else match("J$,l")      add_instr(format_j(KIND_JUMP,  V1, add_reference(line[3])));
    else match("J$[i]")     add_instr(format_i(KIND_JUMP_REG,  0, 0, V1, V3));
    else match("J$,$[i]")   add_instr(format_i(KIND_JUMP_REG,  0, V1, V3, V5));
    else match("jl")        add_instr(format_j(KIND_JUMP,  30, add_reference(line[1])));
    else match("r")         add_instr(format_i(KIND_JUMP_REG,  0, 0, 30, 0));
    else match("l=i")       define_constant(line[0],V2);
    else match("M$,$,$")    add_instr(format_r(KIND_MUL, V0, V1, V3, V5, 0));
    else match("M$,$")      add_instr(format_r(KIND_MUL, V0, V1, V1, V3, 0));
    else match("M$,$,i")    add_instr(format_i(KIND_MUL_LIT, V0, V1, V3, V5));
    else match("M$,i")      add_instr(format_i(KIND_MUL_LIT, V0, V1, V1, V3));
    else match("C$,#,$")    add_instr(format_i(KIND_CFG, V0, V1, V5, V3));
    else match("D$,#")      add_instr(format_i(KIND_CFG, 1,  V1, 0, V3));
    else match("D#,$")      add_instr(format_i(KIND_CFG, 0,  0, V3, V1));
    else match("D$,#,$")    add_instr(format_i(KIND_CFG, 0,  V1, V5, V3));
    else match("Yi")        add_instr(format_i(KIND_CFG, CFG_CMD_SYS,  0, 0, V1));
    else match("J#")        add_instr(format_i(KIND_CFG, 3,  0, 0, V1));
    else match("oi")        origin = V1;
    else if (line[0]->kind=='d')  process_dc(line);

    else                    error("Unrecognized instruction %s",get_line_format(line));
}

/// -----------------------------------------------------------
///                      placeStrings
/// -----------------------------------------------------------

void place_strings() {
    for(token* str=all_strings; str; str=str->next) {
        str->value = prog_length+4;
        str->flags |= FLAG_DEFINED;

        string text = str->name + 1; // Set point after initial "
        int len = strlen(text)-1;  // length excluding trailing "
        add_instr(len);
        while(len>0) {
            if (len==1)
                add_instr(text[0]);
            else if (len==2)
                add_instr(text[0] | (text[1]<<8));
            else if (len==3)
                add_instr(text[0] | (text[1]<<8) | (text[2]<<16));
            else
                add_instr(text[0] | (text[1]<<8) | (text[2]<<16) | (text[3]<<24));
            len -= 4;
            text += 4;
        }

        if (len==0)
            add_instr(0);
    }
}


/// -----------------------------------------------------------
///                      assemble_file
/// -----------------------------------------------------------

void assemble_file(string file_name) {
    open_file(file_name);
    token** line;
    while((line=read_line()))
        assemble_line(line);

}

/// -----------------------------------------------------------
///                      output_file
/// -----------------------------------------------------------

void output_file(string file_name) {
    place_strings();
    resolve_references();

    FILE *fh = fopen(file_name,"w");
    if (fh==0)
        fatal("Cannot open file '%s'",file_name);
    
    for(int k=0; k<prog_length/4; k++)
        fprintf(fh,"%08X\n",prog[k]);

    fclose(fh);

    fh = fopen("out.bin","wb");
    fwrite(prog, prog_length, 1, fh);
    fclose(fh);

    // output labels
    fh = fopen("labels.txt","w");
    for(token *label = all_labels; label; label=label->next)
        if (label->kind=='l')
            fprintf(fh,"%08x %s\n",label->value, label->name);
    for(token *label = all_strings; label; label=label->next)
        fprintf(fh,"%08x %s\n",label->value, label->name);
    fclose(fh);
}

