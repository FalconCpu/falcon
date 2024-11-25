`timescale 1ns/1ns

module cpu(
    input               clock,
    input               reset,

    // Data bus
    output              cpu_request,
    output [31:0]       cpu_address,
    output              cpu_write,
    output [3:0]        cpu_wstrb,
    output [31:0]       cpu_wdata,
    input  [31:0]       cpu_rdata,
    input               cpu_ack,

    output [31:0]       instr_addr,
    input  [31:0]       instr_data,
    output [31:0]       debug_pc
);

// Signals from pc 
wire [31:0] p1_pc;
wire [31:0] p2_pc;

// Signals from icache
wire [31:0] p2_instr;
wire        p2_ack;

// Signals from decoder
wire [4:0]    p2_reg_a;
wire [4:0]    p2_reg_b;
wire          p2_bypass_a3;
wire          p2_bypass_b3;
wire          p2_bypass_a4;
wire          p2_bypass_b4;
wire          p2_b_is_const;
wire [31:0]   p2_const;
wire [4:0]    p4_reg_d;
wire [8:0]    p3_op;
wire [8:0]    p4_op;
wire          p2_pipeline_bubble;


// signals from regfile
wire [31:0]   p3_data_a;
wire [31:0]   p3_data_b;
wire [31:0]   p3_data_c;

// signals from alu
wire [31:0]   p3_out;
wire [31:0]   p4_out;
wire          p3_jump;
wire [31:0]   p3_jump_target;
wire          stall;

assign instr_addr = p1_pc;
assign debug_pc = p2_pc;

cpu_pc  cpu_pc_inst (
    .clock(clock),
    .reset(reset),
    .stall(stall),
    .p2_pipeline_bubble(p2_pipeline_bubble),
    .p3_jump(p3_jump),
    .p3_jump_target(p3_jump_target),
    .p1_pc(p1_pc),
    .p2_pc(p2_pc)
  );

cpu_decoder  cpu_decoder_inst (
    .clock(clock),
    .reset(reset),
    .stall(stall),
    .p2_instr(instr_data),
    .p2_reg_a(p2_reg_a),
    .p2_reg_b(p2_reg_b),
    .p2_bypass_a3(p2_bypass_a3),
    .p2_bypass_b3(p2_bypass_b3),
    .p2_bypass_a4(p2_bypass_a4),
    .p2_bypass_b4(p2_bypass_b4),
    .p2_b_is_const(p2_b_is_const),
    .p2_const(p2_const),
    .p4_reg_d(p4_reg_d),
    .p3_op(p3_op),
    .p4_op(p4_op),
    .p3_jump(p3_jump),
    .p2_pipeline_bubble(p2_pipeline_bubble)
);

cpu_regfile  cpu_regfile_inst (
    .clock(clock),
    .stall(stall),
    .p2_const(p2_const),
    .p2_reg_a(p2_reg_a),
    .p2_reg_b(p2_reg_b),
    .p2_bypass_a3(p2_bypass_a3),
    .p2_bypass_b3(p2_bypass_b3),
    .p2_bypass_a4(p2_bypass_a4),
    .p2_bypass_b4(p2_bypass_b4),
    .p2_b_is_const(p2_b_is_const),
    .p3_data_a(p3_data_a),
    .p3_data_b(p3_data_b),
    .p3_data_c(p3_data_c),
    .p4_reg_d(p4_reg_d),
    .p3_out(p3_out),
    .p4_out(p4_out)
  );

cpu_alu  cpu_alu_inst (
    .clock(clock),
    .reset(reset),
    .stall(stall),
    .p2_pc(p2_pc),
    .p3_op(p3_op),
    .p4_op(p4_op),
    .p3_data_a(p3_data_a),
    .p3_data_b(p3_data_b),
    .p3_data_c(p3_data_c),
    .p3_out(p3_out),
    .p4_out(p4_out),
    .p3_jump(p3_jump),
    .p3_jump_target(p3_jump_target),
    .cpu_request(cpu_request),
    .cpu_address(cpu_address),
    .cpu_write(cpu_write),
    .cpu_wstrb(cpu_wstrb),
    .cpu_wdata(cpu_wdata),
    .cpu_rdata(cpu_rdata),
    .cpu_ack(cpu_ack)
);

endmodule
