#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include "f32.h"

int org = 0xffff0000;

extern int line_number;

static int* prog;
static int prog_alloc;
static int prog_count;

static Reference* references;
static int references_alloc;
static int references_count;
extern Token all_labels;

extern string current_block;

// ================================================
//                  add_reference
// ================================================

static void add_reference(Token label) {
    if (references_count == references_alloc) {
        references_alloc *= 2;
        references = realloc(references, references_alloc*sizeof(struct Reference));
        if (references==0)
            fatal("out of memory allocating references");
    }
    references[references_count].address = prog_count*4;
    references[references_count].line_number = line_number;
    references[references_count].label = label;
    references_count++;
}


// ================================================
//                  add_instr
// ================================================

static void add_instr(int instr) {
    if (prog_count == prog_alloc) {
        prog_alloc *= 2;
        prog = realloc(prog, prog_alloc*4);
        if (prog==0)
            fatal("out of memory allocating program");
    }
    prog[prog_count++] = instr;
}

// ================================================
//                  fmt_r
// ================================================

static int fmt_r(int k, int i, int d, int a, int b) {
    assert(k>=0 && k<=31);
    assert(i>=0 && i<=7);
    assert(d>=0 && d<=31);
    assert(a>=0 && a<=31);
    assert(b>=0 && b<=31);

    return (k<<26) | (i<<23) | (d<<18) | (a<<13) | (b<<0);
}

// ================================================
//                  fmt_i
// ================================================

static int fmt_i(int k, int i, int d, int a, int c) {
    assert(k>=0 && k<=31);
    assert(i>=0 && i<=7);
    assert(d>=0 && d<=31);
    assert(a>=0 && a<=31);
    if (c<-0x1000 || c>=0x1000)
        error("Integer %d out of range", c);

    return (k<<26) | (i<<23) | (d<<18) | (a<<13) | ((c&0x1fff)<<0);
}

// ================================================
//                  fmt_s
// ================================================

static int fmt_s(int k, int i, int a, int b, int c) {
    assert(k>=0 && k<=31);
    assert(i>=0 && i<=7);
    assert(a>=0 && a<=31);
    assert(b>=0 && b<=31);
    if (c<-0x1000 || c>=0x1000)
        error("Integer %d out of range", c);

    return (k<<26) | (i<<23) | ((c&0x1f)<<18) | (a<<13) | (c&0x1fe0) | b;
}

// ================================================
//                  fmt_j
// ================================================

static int fmt_j(int k, int d, int c) {
    assert(k>=0 && k<=31);
    if (c<-0x100000 || c>=0x100000)
        error("Integer %d out of range", c);

    return (k<<26) | (((c>>10)&7)<<23) | (d<<18) | (((c>>5)&0x1f)<<13) | (((c>>13)&0xff)<<5) | (c&0x1f);
}

// ================================================
//               match_format
// ================================================
// look to see if a line matches a format.
// Each token in the line must match the corresponding token in the format.
// But with a couple of exceptions:
//    the format 'r' can match a register or the integer zero

static int match_format(Token* line, char* format) {
    int index = 0;
    for(; format[index]; index++) {

        if (line[index]==0)
            return 0;

        if (format[index]=='r') {
            if (!(line[index]->kind =='$' || (line[index]->kind =='i' && line[index]->value==0)))
                return 0;
        } else
            if (line[index]->kind != format[index])
                return 0;
    }

    if (line[index]!=0)
        return 0;
    return 1;
}

// ================================================
//              generate_load_immediate
// ================================================

static void generate_load_immediate(int d, int c) {
    if (c>=-0x1000 && c<0x1000) {
        add_instr(fmt_i(KIND_ALUI, 1, d, 0, c));
    } else {
        add_instr(fmt_j(KIND_LDU, d, c>>11));
        if (c & 0xfff)
            add_instr(fmt_i(KIND_ALUI, 1, d, d, c&0xfff));
    }
}

// ================================================
//                  is_label_global
// ================================================
// test to see if a label name doesn't contain a dot or @

static int is_label_global(Token label) {
    for(int i=0; label->text[i]; i++)
        if (label->text[i]=='.' || label->text[i]=='@')
            return 0;
    return 1;
}


// ================================================
//                  define_label
// ================================================

static void define_label(Token label) {
    if (label->flags & FLAG_DEFINED)
        error("Duplicate label %s", label->text);
    label->flags |= FLAG_DEFINED;
    label->value = org + prog_count*4;

    if (is_label_global(label))
        current_block = label->text;
}

// ================================================
//                  define_equ
// ================================================

static void define_equ(Token label, Token value) {
    if (label->flags & FLAG_DEFINED)
        error("Duplicate label %s", label->text);
    label->kind = 'i';
    label->value = value->value;
}

// ================================================
//                  define_space
// ================================================

static void define_space(int value) {
    if (value&3)
        error("Space must be a multiple of 4");
    value = value/4;
    while (value--)
        add_instr(0);
}

// ================================================
//                  generate_dc
// ================================================

static int build_word;
static int build_count;

static void out_byte(int c) {
    build_word |= (c&0xff)<<(8*build_count);
    build_count++;
    if (build_count==4) {
        add_instr(build_word);
        build_word = 0;
        build_count = 0;
    }
}

static void clear_out_word() {
    if (build_count)
        add_instr(build_word);
    build_word = 0;
    build_count = 0;
}

static void generate_dcb(Token* line) {
    clear_out_word();
    int i = 0;
    while(1) {
        switch(line[i]->kind) {
            case 'i':
                if (line[i]->value<-128 || line[i]->value>255 )
                    error("Invalid byte %d", line[i]->value);
                out_byte(line[i]->value);
                break;

            case '"':
                for (int j=1; line[i]->text[j+1]; j++)
                    out_byte(line[i]->text[j]);
                break;

            default:
                error("Invalid byte");
        }
        i++;

        if (line[i]==0)
            break;
        if (line[i]->kind==',')
            i++;
        else
            error("Invalid byte");
    }
    clear_out_word();
}

static void generate_dch(Token* line) {
    clear_out_word();
    int i = 0;
    while(1) {
        switch(line[i]->kind) {
            case 'i':
                if (line[i]->value<-0x8000 || line[i]->value>=0x8000)
                    error("Invalid halfword %d", line[i]->value);
                out_byte(line[i]->value & 0xff);
                out_byte((line[i]->value >> 8) & 0xff);
                break;

            default:
                error("Invalid halfword");
        }
        i++;

        if (line[i]==0)
            break;
        if (line[i]->kind==',')
            i++;
        else
            error("Invalid byte");
    }
    clear_out_word();
}

static void generate_dcw(Token* line) {
    int i = 0;
    while(1) {
        switch(line[i]->kind) {
            case 'i':
                add_instr(line[i]->value);
                break;

            case 'l':
                add_reference(line[i]);
                add_instr(0);
                break;

            default:
                error("Invalid word");
        }

        i++;
        if (line[i]==0)
            break;
        if (line[i]->kind==',')
            i++;
        else
            error("Invalid word");
    }
}

static void generate_dc(Token* line) {
    switch (line[0]->value) {
        case 0: generate_dcb(line+1); break;
        case 1: generate_dch(line+1); break;
        case 2: generate_dcw(line+1); break;
        default: fatal("Invalid dc");
    }
}

// ================================================
//                  assemble_line
// ================================================

static void assemble_line(Token* line) {
    
    if (line[0]!=0 && line[1]!=0 && line[0]->kind=='l' && line[1]->kind==':') {
        define_label(line[0]);
        line += 2;
    }

    #define CASE(X) else if (match_format(line, X)) 
    #define V0 line[0]->value
    #define V1 line[1]->value
    #define V2 line[2]->value
    #define V3 line[3]->value
    #define V4 line[4]->value
    #define V5 line[5]->value

    if (line[0]==0)         { return; }
    else if (line[0]->kind=='d')  { generate_dc(line); }
    CASE("A$,r,r")          { add_instr(fmt_r(KIND_ALU, V0, V1, V3, V5)); }
    CASE("A$,r")            { add_instr(fmt_r(KIND_ALU, V0, V1, V1, V3)); }
    CASE("A$,i")            { add_instr(fmt_i(KIND_ALUI, V0, V1, V1, V3)); }
    CASE("A$,r,i")          { add_instr(fmt_i(KIND_ALUI, V0, V1, V3, V5)); }
    CASE("H$,r,i")          { add_instr(fmt_i(KIND_ALUI, 3, V1, V3, (V0<<5) | V5)); }
    CASE("H$,i")            { add_instr(fmt_i(KIND_ALUI, 3, V1, V1, (V0<<5) | V3)); }
    CASE("H$,r,$")          { add_instr(fmt_i(KIND_ALU, 3, V1, V3, (V0<<5) | V5)); }
    CASE("H$,$")            { add_instr(fmt_i(KIND_ALU, 3, V1, V1, (V0<<5) | V3)); }
    CASE("D$,r")            { add_instr(fmt_i(KIND_ALU,  1, V1, V3, 0)); }
    CASE("D$,i")            { generate_load_immediate(V1, V3); }
    CASE("L$,r[i]")         { add_instr(fmt_i(KIND_LD, V0, V1, V3, V5)); }
    CASE("Sr,r[i]")         { add_instr(fmt_s(KIND_ST, V0, V3, V1, V5)); }
    CASE("Br,r,l")          { add_reference(line[5]); add_instr(fmt_s(KIND_BRA, V0, V1, V3, 0)); }
    CASE("Jl")              { add_reference(line[1]); add_instr(fmt_j(KIND_JMP, 0, 0)); }
    CASE("J$[i]")           { add_instr(fmt_i(KIND_JMPR, 0, 0, V1, V3)); }
    CASE("J$")              { add_instr(fmt_i(KIND_JMPR, 0, 0, V1, 0)); }
    CASE("jr,$[i]")         { add_instr(fmt_i(KIND_JMPR, 0, V1, V3, V5)); }
    CASE("jl")              { add_reference(line[1]); add_instr(fmt_j(KIND_JMP, 30, 0)); }  // jsr instruction
    CASE("k")               { add_instr(fmt_i(KIND_JMPR, 0, 0, 30, 0)); }       // ret instruction
    CASE("jr,l")            { add_reference(line[3]); add_instr(fmt_j(KIND_JMP, V1, 0)); }
    CASE("D$,l")            { add_reference(line[3]); add_instr(fmt_j(KIND_LDPC, V1, 0)); }
    CASE("l=i")             { define_equ(line[0], line[2]); }
    CASE("zi")              { define_space(V1); }
    CASE("M$,r")            { add_instr(fmt_r(KIND_MUL, V0, V1, V1, V3)); }
    CASE("M$,r,r")          { add_instr(fmt_r(KIND_MUL, V0, V1, V3, V5)); }
    CASE("M$,i")            { add_instr(fmt_i(KIND_MULI, V0, V1, V1, V3)); }
    CASE("M$,r,i")          { add_instr(fmt_i(KIND_MULI, V0, V1, V3, V5)); }
    CASE("C$,!")            { add_instr(fmt_i(KIND_CFG, 0, V1, 0, V3)); }
    CASE("C!,$")            { add_instr(fmt_i(KIND_CFG, 1, 0, V3, V1)); }
    CASE("C$,!,$")          { add_instr(fmt_i(KIND_CFG, 1, V1, V5, V3)); }
    else                    { error("Unrecognized instruction"); for(int k=0; line[k]; k++) printf("%c", line[k]->kind); printf("\n"); }
}


// ================================================
//                  resolve_references
// ================================================

static void resolve_reference(Reference* reference) {
    line_number = reference->line_number;
    int addr = reference->address / 4;   // Convert from bytes to words
    Token label = reference->label;

    if (! (label->flags & FLAG_DEFINED)) {
        error("Undefined label '%s'", label->text);
        return;
    }
    
    int instr_kind = (prog[addr] >> 26) & 0x1f;
    int instr_i = (prog[addr] >> 23) & 0x7;
    int instr_d = (prog[addr] >> 18) & 0x1f;
    int instr_a = (prog[addr] >> 13) & 0x1f;
    int instr_b = (prog[addr] >> 0) & 0x1f;

    int offset = (label->value - org - reference->address - 4)/4;
    switch(instr_kind) {
        case 0:        prog[addr] = label->value; break;
        case KIND_JMP: prog[addr] = fmt_j(instr_kind, instr_d, offset); break;
        case KIND_BRA: prog[addr] = fmt_s(instr_kind, instr_i, instr_a, instr_b, offset); break;
        case KIND_LDPC: prog[addr] = fmt_j(instr_kind, instr_d, offset); break;
        default: error("Can't resolve reference to instruction");
    }
}

static void resolve_references() {
    int i;
    for(i=0; i<references_count; i++)
        resolve_reference(& references[i]);
 }

// ================================================
//                  output_labels
// ================================================

static void output_labels() {
    FILE *fh = fopen("asm.labels", "w");
    if (fh==0) {
        error("Can't open file '%s'", "asm.labels");
        return;
    }

    for(Token ptr = all_labels; ptr; ptr = ptr->next)
        if (ptr->flags & FLAG_DEFINED)
            fprintf(fh, "%x %s\n", ptr->value, ptr->text);
    fclose(fh);
}

// ================================================
//             initialize_assembler
// ================================================

void initialize_assembler() {
    prog_alloc = 1024;
    prog_count = 0;
    prog = my_malloc(prog_alloc * sizeof(int));

    references_alloc = 1024;
    references_count = 0;
    references = my_malloc(references_alloc * sizeof(Reference));
}

// ================================================
//                  assemble_file
// ================================================

void assemble_file(string filename) {
    open_file(filename);
    Token* line;
    while ((line = read_line()) != 0) {
        assemble_line(line);
    }

}

// ================================================
//                  output_result
// ================================================

void output_result(string filename) {
    resolve_references();

    FILE *fh = fopen(filename, "w");
    if (fh==0) {
        error("Can't open file '%s'", filename);
        return;
    }

    int i;
    for(i=0; i<prog_count; i++)
        fprintf(fh,"%08x\n", prog[i]);

    fclose(fh);
    output_labels();
}