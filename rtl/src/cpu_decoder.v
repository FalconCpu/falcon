`include "cpu.vh"
`timescale 1ns/1ps

module cpu_decoder(
    input                clock,
    input                reset,
    input                stall,
    input      [31:0]    instr_data,

    output     [4:0]     p2_reg_a,       // register number for 'a' source
    output     [4:0]     p2_reg_b,       // register number for 'b' source
    output reg [1:0]     p2_src_data_a,  // which data source to use for alu 
    output reg [1:0]     p2_src_data_b,  // which data source to use for alu 
    output reg [1:0]     p2_src_addr_a,  // which data source to use for address gen
    output reg [1:0]     p2_src_addr_b,  // which data source to use for address gen
    output reg [31:0]    p2_literal,
    output reg           p2_bubble,      // indicate we are unable to accept an instruction this cycle

    input                p3_jump, 
    input                p3_exception,   // Instruction at p3 has triggered an exception
    output reg [6:0]     p3_op,          // operation to perform
    output reg [1:0]     p3_opx,         // extra bits to specify shift operations
    output reg [4:0]     p3_dest_reg,    // register to write data to
    output reg [4:0]     p4_dest_reg,    // register to write data to
    output reg [31:0]    p3_instr        // copy of instruction (for use in illegal instruction exception)
);

// Split the instruction up into its constituent fields
// 109876 543 21098 76543 21098765 43210
// KKKKKK III DDDDD AAAAA CCCCCCCC BBBBB
wire [5:0] instr_k = instr_data[31:26];
wire [2:0] instr_i = instr_data[25:23];
wire [4:0] instr_d = instr_data[22:18];
wire [4:0] instr_a = instr_data[17:13];
wire [7:0] instr_c = instr_data[12:5];
wire [4:0] instr_b = instr_data[4:0];

// build up the literal constants
wire [31:0] lit_s13  = {{19{instr_c[7]}}, instr_c, instr_b};
wire [31:0] lit_s13d = {{19{instr_c[7]}}, instr_c, instr_d};
wire [31:0] lit_s21  = {{9{instr_c[7]}}, instr_c, instr_i, instr_a, instr_b, 2'b0};

assign p2_reg_a = instr_a;
assign p2_reg_b = instr_b;

reg [6:0] p2_op;
reg [1:0] p2_opx;
reg [4:0] p2_dest_reg;
reg       p2_latent, p3_latent;   // indicates an instruction that takes more than 1 clock cycle to complete
reg       p2_illegal;


always @(*) begin
    // 
    // Decode the instruction and determine data sources for ALU, and address generator. 
    //

    // default values 
    p2_latent     = 1'b0;
    p2_opx        = 2'b0;
    p2_src_addr_a = `SRC_ZERO;
    p2_src_addr_b = `SRC_ZERO;
    p2_src_data_a = `SRC_ZERO;
    p2_src_data_b = `SRC_ZERO;
    p2_dest_reg   = 5'b0;
    p2_literal    = 32'bx;     
    p2_bubble     = 1'b0;           
    p2_illegal    = 1'b0;

    if (p3_jump || reset) begin
        // Instruction immediately after a jump or taken branch gets nullified - treat it as if it were
        // an AND instruction with the null register as the destination
        p2_op = `OP_AND;

    end else if (p3_latent && (p3_dest_reg==instr_a || p3_dest_reg==instr_b)) begin
        // If the instruction might access the register written to by the instruction currently in the ALU
        // and the ALU isn't guaranteed to have its result availible for forwarding
        p2_op = `OP_AND;
        p2_bubble = 1'b1;

    end else case(instr_k)
        `INSTR_ALU_REG   : begin
            p2_op         = {4'b0, instr_i};
            p2_opx        = (instr_i==3'h3) ? instr_c[1:0] : 2'b0;
            p2_src_data_a = `SRC_REG;
            p2_src_data_b = `SRC_REG;
            p2_dest_reg   = instr_d;
        end

        `INSTR_ALU_LIT   : begin
            p2_op         = {4'b0, instr_i};
            p2_opx        = (instr_i==3'h3) ? instr_c[1:0] : 2'b0;
            p2_src_data_a = `SRC_REG;
            p2_src_data_b = `SRC_LIT;
            p2_dest_reg   = instr_d;                
            p2_literal    = lit_s13;                
        end

        `INSTR_LOAD      : begin
            p2_op         = {4'h2, instr_i};
            p2_src_addr_a = `SRC_REG;
            p2_src_addr_b = `SRC_LIT;
            p2_dest_reg   = instr_d;                
            p2_literal    = lit_s13;                
            p2_latent     = 1'b1;
        end

        `INSTR_STORE     : begin
            p2_op         = {4'h3, instr_i};
            p2_src_data_a = `SRC_ZERO;
            p2_src_data_b = `SRC_REG;
            p2_src_addr_a = `SRC_REG;
            p2_src_addr_b = `SRC_LIT;
            p2_dest_reg   = 5'h0;                
            p2_literal    = lit_s13d;                
        end

        `INSTR_BRANCH    : begin
            p2_op         = {4'h4, instr_i};
            p2_src_data_a = `SRC_REG;
            p2_src_data_b = `SRC_REG;
            p2_src_addr_a = `SRC_PC;
            p2_src_addr_b = `SRC_LIT;
            p2_dest_reg   = 5'h0;                
            p2_literal    = lit_s13d << 2;                
        end

        `INSTR_JUMP      : begin
            p2_op         = `OP_JMP;
            p2_src_data_a = `SRC_PC;
            p2_src_data_b = `SRC_ZERO;
            p2_src_addr_a = `SRC_PC;
            p2_src_addr_b = `SRC_LIT;
            p2_dest_reg   = instr_d;                
            p2_literal    = lit_s21;                
        end

        `INSTR_JUMP_REG  : begin
            p2_op         = `OP_JMP;
            p2_src_data_a = `SRC_PC;
            p2_src_addr_a = `SRC_REG;
            p2_src_addr_b = `SRC_LIT;
            p2_dest_reg   = instr_d;                
            p2_literal    = lit_s13;                
        end

        `INSTR_LDU       : begin
            p2_op         = `OP_OR;
            p2_opx        = 2'b0;
            p2_src_data_b = `SRC_LIT;
            p2_dest_reg   = instr_d;                
            p2_literal    = lit_s21 << 9;                
        end

        `INSTR_LDPC      : begin
            p2_op         = `OP_ADD;
            p2_opx        = 2'b0;
            p2_src_data_a = `SRC_PC;
            p2_src_data_b = `SRC_LIT;
            p2_dest_reg   = instr_d;                
            p2_literal    = lit_s21;                
        end

        `INSTR_CFG:        begin
            p2_op         = {4'h9, instr_i};
            p2_src_data_a = `SRC_REG;
            p2_src_data_b = `SRC_LIT;
            p2_literal    = lit_s13;                
            p2_dest_reg   = instr_d;                        
        end

        `INSTR_MUL: begin
            p2_op         = {4'hA, instr_i};
            p2_src_data_a = `SRC_REG;
            p2_src_data_b = `SRC_REG;
            p2_dest_reg   = instr_d;
            p2_latent     = 1'b1;
        end

        `INSTR_MUL_LIT: begin
            p2_op         = {4'hA, instr_i};
            p2_src_data_a = `SRC_REG;
            p2_src_data_b = `SRC_LIT;
            p2_dest_reg   = instr_d;                
            p2_literal    = lit_s13;                
            p2_latent     = 1'b1;
        end

        default: begin
            // illegal instructions
            p2_op         = `OP_ILLEGAL;
            p2_opx        = 2'b0;
            p2_dest_reg   = 5'b0;   
            p2_illegal    = 1'b1;

        end

    endcase

    if (p2_illegal) begin
        p2_op = `OP_ILLEGAL;
        p2_dest_reg   = 5'b0;   
    end

end


always @(posedge clock)
    if (!stall) begin
        p3_op       <= p2_op;
        p3_opx      <= p2_opx;
        p3_latent   <= p2_latent;
        p3_dest_reg <= p2_dest_reg;
        p4_dest_reg <= p3_exception ? 5'h0 : p3_dest_reg;
        p3_instr    <= instr_data;
    end
endmodule