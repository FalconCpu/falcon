`timescale 1ns / 1ns

module cpu_pc(
    input               clock,
    input               reset,
    input               stall,
    input               p2_pipeline_bubble,

    // Connection to execution unit
    input               p3_jump,
    input [31:0]        p3_jump_target,

    // Connection to instruction bus
    output reg [31:0]   p1_pc,
    output reg [31:0]   p2_pc,
    output reg [31:0]   p3_pc,

    input  [31:0]       instr_data,
    output [31:0]       instr_data_out
);

reg prev_stall;
reg [31:0] instr_data_skid_buffer;

// By the time the pipeline detects a stall - it may be too late to correct the
// address we send to the instruction cache. So instead we use a skid buffer
// to store the current instruction data and send it out on the next cycle.
// This allows the address that gets sent out to be garbage - as we don't
// use the instruction data we get back anyway.
assign  instr_data_out = prev_stall ? instr_data_skid_buffer : instr_data;

always @(*) begin
    if (p3_jump)
        p1_pc = p3_jump_target;
    else
        p1_pc = p2_pc + 4;

    if (reset) begin
        p1_pc = 32'hffff0000;
    end
end

always @(posedge clock) begin
    prev_stall <= stall || p2_pipeline_bubble;
    if (!prev_stall) 
        instr_data_skid_buffer <= instr_data;

    if (!stall && !p2_pipeline_bubble || reset) begin
        p2_pc <= p1_pc;
        p3_pc <= p2_pc;
    end
end

// synthesis translate_off
always @(posedge clock) begin
    if (p2_pc == 32'h0) begin
        $finish();
    end
end
// synthesis translate_on


endmodule