`timescale 1ns / 1ns
`include "f32.vh"

module cpu_decoder(
    input               clock,
    input               reset,
    input               stall,

    // connections from the icache
    input [31:0]        p2_instr,

    // connections to the datamux
    output     [4:0]    p2_reg_a,
    output     [4:0]    p2_reg_b,
    output reg          p2_bypass_a3,
    output reg          p2_bypass_b3,
    output reg          p2_bypass_a4,
    output reg          p2_bypass_b4,
    output reg          p2_b_is_const,
    output reg [31:0]   p2_const,
    output reg [4:0]    p4_reg_d,

    // connectiorn to the execution unit
    output reg [8:0]    p3_op,
    output reg [8:0]    p4_op,
    input               p3_jump,
    output reg          p2_pipeline_bubble
);

// Decode the instruction into fields
// 109876543 21098 76543 21098765 43210
// KKKKKKKKK DDDDD AAAAA CCCCCCCC BBBBB

wire [8:0] p2_k  = p2_instr[31:23];
wire [4:0] p2_d  = p2_instr[22:18];
wire [4:0] p2_a  = p2_instr[17:13];
wire [7:0] p2_c  = p2_instr[12:5];
wire [4:0] p2_b  = p2_instr[4:0];

// determine the datasource for the instruction
assign p2_reg_a = p2_a;
assign p2_reg_b = p2_b;
reg p2_a_is_reg;
reg p2_b_is_reg;



// Decode the instruction
reg [4:0] p2_reg_d, p3_reg_d;
reg [8:0] p2_op;

reg p2_is_latent, p3_is_latent;


always @(*) begin
    p2_is_latent = 0;
    p2_pipeline_bubble = 0;

    if (p3_jump) begin 
        p2_op         = 0;
        p2_a_is_reg   = 1'bx;
        p2_b_is_reg   = 1'bx;
        p2_b_is_const = 1'bx;
        p2_reg_d      = 0;
        p2_const      = 32'bx;

    end else casex(p2_k) 
        `INST_ALU: begin
            p2_op         = {6'h10, p2_k[2:0]};
            p2_a_is_reg   = 1;
            p2_b_is_reg   = 1;
            p2_b_is_const = 0;
            p2_reg_d      = p2_d;
            p2_const      = {{19{p2_c[7]}}, p2_c, p2_b};  // The different shift instructions are encoded as constants
        end

        `INST_ALUI: begin
            p2_op         =  {6'h10, p2_k[2:0]};
            p2_a_is_reg   = 1;
            p2_b_is_reg   = 0;
            p2_b_is_const = 1;
            p2_reg_d      = p2_d;
            p2_const      = {{19{p2_c[7]}}, p2_c, p2_b};
        end

        `INST_LD: begin
            p2_op         =  {6'h12, p2_k[2:0]};
            p2_a_is_reg   = 1;
            p2_b_is_reg   = 0;
            p2_b_is_const = 0;
            p2_reg_d      = p2_d;
            p2_const      = {{19{p2_c[7]}}, p2_c, p2_b};
            p2_is_latent  = 1;
        end

        `INST_ST: begin
            p2_op         =  {6'h13, p2_k[2:0]};
            p2_a_is_reg   = 1;
            p2_b_is_reg   = 1;
            p2_b_is_const = 0;
            p2_reg_d      = 0;
            p2_const      = {{19{p2_c[7]}}, p2_c, p2_d};
        end

        `INST_BRA: begin
            p2_op         =  {6'h14, p2_k[2:0]};
            p2_a_is_reg   = 1;
            p2_b_is_reg   = 1;
            p2_b_is_const = 0;
            p2_reg_d      = 0;
            p2_const      = {{17{p2_c[7]}}, p2_c, p2_d, 2'b00};
        end

        `INST_JMP: begin
            p2_op         =  {6'h15, 3'b0};
            p2_a_is_reg   = 0;
            p2_b_is_reg   = 0;
            p2_b_is_const = 0;
            p2_reg_d      = p2_d;
            p2_const      = {{9{p2_c[7]}}, p2_c, p2_instr[25:23], p2_a, p2_b,2'b0};
        end

        `INST_JMPR: begin
            p2_op         =  {6'h16, 3'b0};
            p2_a_is_reg   = 1;
            p2_b_is_reg   = 0;
            p2_b_is_const = 0;
            p2_reg_d      = p2_d;
            p2_const      = {{19{p2_c[7]}}, p2_c, p2_b};
        end

        `INST_LDU: begin
            p2_op         =  {6'h17, 3'b0};
            p2_a_is_reg   = 0;
            p2_b_is_reg   = 0;
            p2_b_is_const = 1;
            p2_reg_d      = p2_d;
            p2_const      = {p2_c, p2_instr[25:23], p2_a, p2_b,11'b0};
        end

        `INST_LDPC: begin
            p2_op         =  {6'h18, 3'b0};
            p2_a_is_reg   = 0;
            p2_b_is_reg   = 0;
            p2_b_is_const = 1;
            p2_reg_d      = p2_d;
            p2_const      = {{9{p2_c[7]}}, p2_c, p2_instr[25:23], p2_a, p2_b,2'b0};
        end

        `INST_MULR: begin
            p2_op         = {6'h19, p2_k[2:0]};
            p2_a_is_reg   = 1;
            p2_b_is_reg   = 1;
            p2_b_is_const = 0;
            p2_reg_d      = p2_d;
            p2_const      = 32'bx;
            p2_is_latent  = 1;
        end

        `INST_MULI: begin
            p2_op         = {6'h19, p2_k[2:0]};
            p2_a_is_reg   = 1;
            p2_b_is_reg   = 0;
            p2_b_is_const = 1;
            p2_reg_d      = p2_d;
            p2_const      = {{19{p2_c[7]}}, p2_c, p2_b};
            p2_is_latent  = 1;
        end

        `INST_CFG: begin
            p2_op         = {6'h1b, p2_k[2:0]};
            p2_a_is_reg   = 1;
            p2_b_is_reg   = 0;
            p2_b_is_const = 1;
            p2_reg_d      = p2_d;
            p2_const      = {{19{p2_c[7]}}, p2_c, p2_b};
            p2_is_latent  = 0;
        end

        default: begin
            p2_op         = 9'b0;
            p2_a_is_reg   = 0;
            p2_b_is_reg   = 0;
            p2_b_is_const = 0;
            p2_reg_d      = 0;
            p2_const      = 32'bx;
        end
    endcase

    p2_bypass_a3 <= p2_a_is_reg && p3_reg_d == p2_a && p2_a != 0;
    p2_bypass_b3 <= p2_b_is_reg && p3_reg_d == p2_b && p2_b != 0;
    p2_bypass_a4 <= p2_a_is_reg && p4_reg_d == p2_a && p2_a != 0;
    p2_bypass_b4 <= p2_b_is_reg && p4_reg_d == p2_b && p2_b != 0;

    if (p3_is_latent && (p2_bypass_a3 || p2_bypass_b3)) begin
        p2_pipeline_bubble <= 1;
        p2_op <= 0;
        p2_reg_d <= 0;
    end
end

// registers
always @(posedge clock) begin
    if (!stall) begin
        p3_op    <= p2_op;
        p4_op    <= p3_op;
        p3_is_latent <= p2_is_latent;
        p3_reg_d <= p2_reg_d;
        p4_reg_d <= p3_reg_d;
    end
end

endmodule