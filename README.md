# Falcon project

As a retirement project I have set myself the challenge of designing a computer system from scratch. 

Sub projects of this include:

* Design my own CPU instruction set (Falcon 32), and the basic tools to support it - an assembler, disassembler and emulator
* Create my own programming language (FPL - Falcon Programming Language), and write a compiler to compile it targeting my CPU 
* Implement the CPU and peripherals in verilog rtl. To run on a DE1-SOC fpga board.
* Write a multi-tasking operating system (Falcon OS) written in FPL, running on the F32.
* Write some apps (a drawing program, some games) that can run on the above sysyem.

This should keep me busy for a little while. This is likely to be continually evolving as I learn - and lets see where things go!


## The F32 CPU

I have gone for a fairly standard 32 bit RISC architecture, based somewhat on RISC-V, but with encodings that made sense to me.

The CPU has 31 regsiters named $1..$31 (register zero hardcoded to 0).   $31 is conventinally the stack pointer. 

All instructions are 32 bits in length. The instructions are broken up into the following fields:-
```
   332222 222 22211 11111 111
   109876 543 21098 76543 21098765 43210   
   KKKKKK III DDDDD AAAAA CCCCCCCC BBBBB

Where the fields have the following meaning:-
+ KKKKKK = 6 bit field indicating the 'kind' of the instruction: ALU, LOAD, STORE, etc
+ III    = 3 bit field giving more detail to the K field, eg ALU gets broken into ADD/SUB/AND/OR etc
+ DDDDD  = 5 bit field indicating destination register
+ AAAAA  = 5 bit field indicating the first source register
+ BBBBB  = 5 bit field indicating the second source register
+ CCCCC  = 8 bit field, usually used for literal values, but for specific meanings for some instructions

Literal values are formed by concatenating the above fields together in different ways. They are always interpreted
as signed 2's complement, and the CCC field is always at the most significant end of the literal.

#S13 = 13 bit literal formed by concatenating CCCCC and BBBBB
#S13D = 13 bit literal formed by concatenating CCCCCC and DDDDD
#S21  = 21 bit literal formed by concatenating CCCCCC III AAAAA BBBBBB

So the instruction kinds are

109876 543 21098 76543 21098765 43210   
010000 III DDDDD AAAAA ........ BBBBB   alu3 $d,$a,$b        ALU ops: and/or/xor/shift/add/sub/clt/cltu
010001 III DDDDD AAAAA ######## #####   alu3 $d,$a,#s13      ALU literal ops. III has same meaning as ALU
010010 ZZZ DDDDD AAAAA ######## #####   ldz  $d,$a[#s13]     Memory loads: ldb/ldh/ldw
010011 ZZZ ##### AAAAA ######## BBBBB   stz  $b,$a[#s13d]    Memory stores: stb/sth/stw
010100 CCC ##### AAAAA ######## BBBBB   bcc  $a,$b,#s13d     Branch beq/bne/blt/bge/bltu/bgeu   dest=next_pc+4*s13d
010101 ### DDDDD ##### ######## #####   jmp  $d, #s21        dest=next_pc+4*s21
010110 ... DDDDD AAAAA ######## #####   jmp  $d, $a[#s13]    Jump to address from register + 4*s13
010111 ### DDDDD ##### ######## #####   ld   $d, #21<<11     Load upper bits of register with literal value
011000 ### DDDDD ##### ######## #####   add  $d, $pc, #21    Calculate pc relative address
011001 III DDDDD AAAAA ######## #####   cfg  $d, $a, #reg    load config regs
011010 ### DDDDD AAAAA ........ BBBBB   mul  $d, $a, $b      Multiply ops: mul/../../../divs/mods/divu/modu
```

The assembler, disassembler and emulator are written in 'C' and can be found in the assembler directory.

## The FPL programming language

Depending on your point of view I've taken the best, or worst, bits of my three favorite languages and put them together. 
So FPL is a biasically the semantics of 'C', with the syntax of Kotlin, and the significant whitespace of Python (although
with optional 'end' commands). More details on the language itself in the readme file for the compiler.

The compiler itself is written in C, and runs multiple passes
* Interleaved lexing/parsing to build an Abstract syntax tree, using a recursive descent parser
* Three pass semantic checks, (to allow for forward references)
   * Identify user defined types
   * Identify functions and their parameters
   * Typechecking all expressions and statements
 * Code generation to form a three address Intermediate representation
 * Peephole optimiser to clean things up
 * Maybe other optimisation passes to come 
 * Register allocation
 * Final pass of peephole optimisations
 * Conversion into final assembly language.

The compiler executable has command line switches to cause it to stop and dump out its state at any of these stages

## The hardware

Verilog hardware definition language files to allow this to be realised on an Altera CycloneV FPGA - on a Terassic DE1-SOC development board.

The CPU is implemented in the classic 5 stage pipeline  Instruction Fetch/Decode/Execute/Memory access/Writeback. It is timing closed to run at 100Mhz.
Currently it supports all the instructions in the emulator, but does not have caches (yet).
It has a dedicated 64kB instruction memory, and code can only be run from that memory.
It has basic exception handling, for illegal instructions and misaligned memory addresses. It does not yet have interrupts or supervisor/user modes.

There is also a VGA display adaptor (640x480 in 8 bit indexed color). with a mouse sprite implemented in hardware.
I plan to implement a blitter, and maybe one day a GPU - but not yet.

The CPU and VGA share connections to a 64Mb SDRAM, with a fairly simplisic memory controller, and arbitrator to allow CPU and VGA adapter to share access to the ram (VGA has priority).

Other peripherals are 6 digit seven segment display, 10 LEDs, 10 physical switches, PS2 mouse and keyboard, and UART to communicate with the host PC.
All peripherals memory mapped to be accessible to the CPU.


## The operating system

This is in the very early stages of development so far. Currently only consists of a memory self test, boot loader to load an
image over the UART link, and some simple memory allocation/

## Applications

I currently have the start of three apps written that run on my OS:-

+ A very basic drawing program implemented that allows you to freehand, straight lines and text, in different colors.

+ An image viewer (upload raw bitmap images over the UART and it displays them on the VGA)

+ And the start of a top down car game where you can drive a car around a maze of obstacles. Currently only implemented to the stage where it draws the car.

