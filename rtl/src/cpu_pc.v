`timescale 1ns / 1ns

module cpu_pc(
    input               clock,
    input               reset,
    input               stall,
    input               p2_pipeline_bubble,
    input               p2_instr_valid,

    // Connection to execution unit
    input               p3_jump,
    input [31:0]        p3_jump_target,

    // Connection to instruction bus
    output reg [31:0]   p1_pc,
    output reg [31:0]   p2_pc,
    output reg [31:0]   p3_pc,
    output reg [31:0]   p4_pc
);

always @(*) begin
    if (p3_jump)
        p1_pc = p3_jump_target;
    else
        p1_pc = p2_pc + (p2_instr_valid ? 4 : 0);

    if (reset) begin
        p1_pc = 32'hffff0000;
    end
end

always @(posedge clock) begin
    if (!stall && !p2_pipeline_bubble || reset)
        p2_pc <= p1_pc;

    if (!stall) begin
        p3_pc <= p2_pc;
        p4_pc <= p3_pc;
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