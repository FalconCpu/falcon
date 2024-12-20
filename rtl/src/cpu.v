`timescale 1ns/1ns

module cpu(
    input               clock,
    input               reset,

    // Data bus
    output              cpu_request,    // Cpu requests a memory access
    output [31:0]       cpu_address,    // Address
    output              cpu_write,      // 1 = write, 0 = read
    output [3:0]        cpu_wstrb,      // Byte enables for a write
    output [31:0]       cpu_wdata,      // Data to write
    input  [31:0]       cpu_rdata,      // Data read
    input               cpu_mem_busy,   // Memory bus is busy
    input               cpu_valid,      // rdata is valid

    output [15:0]       imem_addr,
    input  [31:0]       imem_data,
    output [31:0]       debug_pc,

    output              icache_request,     // request to the memory system
    output [25:0]       icache_address,     // Address to read from
    input  [31:0]       icache_rdata,       // data read from sdram
    input               icache_ack,         // transaction is acknowledged 
    input               icache_valid,       // data is valid
    input               icache_complete     // transaction is complete
);

// Signals from pc 
wire [31:0] p1_pc;
wire [31:0] p2_pc;
wire [31:0] p3_pc;
wire [31:0] p4_pc;

// Signals from icache
wire [31:0] p2_instr;
wire        p2_ack;
wire        p2_instr_valid;
wire        p2_inst_invalid_address; // TODO

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
wire [31:0]   p2_instr_data;


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

assign debug_pc = p2_pc;

cpu_pc  cpu_pc_inst (
    .clock(clock),
    .reset(reset),
    .stall(stall),
    .p2_pipeline_bubble(p2_pipeline_bubble),
    .p2_instr_valid(p2_instr_valid),
    .p3_jump(p3_jump),
    .p3_jump_target(p3_jump_target),
    .p1_pc(p1_pc),
    .p2_pc(p2_pc),
    .p3_pc(p3_pc),
    .p4_pc(p4_pc)
  );

  cpu_icache  cpu_icache_inst (
    .clock(clock),
    .reset(reset),
    .stall(stall || p2_pipeline_bubble),
    .inst_address(p1_pc),
    .inst_rdata(p2_instr_data),
    .inst_valid(p2_instr_valid),
    .inst_invalid_address(p2_inst_invalid_address),
    .imem_address(imem_addr),
    .imem_rdata(imem_data),
    .icache_request(icache_request),
    .icache_address(icache_address),
    .icache_rdata(icache_rdata),
    .icache_ack(icache_ack),
    .icache_valid(icache_valid),
    .icache_complete(icache_complete)
  );


cpu_decoder  cpu_decoder_inst (
    .clock(clock),
    .reset(reset),
    .stall(stall),
    .p2_instr(p2_instr_data),
    .p2_instr_valid(p2_instr_valid),
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
    .p3_pc(p3_pc),
    .p4_pc(p4_pc),
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
    .cpu_mem_busy(cpu_mem_busy),
    .cpu_valid(cpu_valid)
);

endmodule
