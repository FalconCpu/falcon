`timescale 1ns/1ps

module cpu_pc(
    input              clock,
    input              reset,
    input              stall,
    input              p2_bubble,     // indicate the decoder is unable to receive an instruction

    input              p3_jump,
    input      [31:0]  p3_jump_addr,
 
    output reg [31:0]  instr_address,
    output reg [31:0]  p1_pc,
    output reg [31:0]  p2_pc,
    output reg [31:0]  p3_pc
);

always @(*) begin
    // for the value of 'pc' seen by an instruction 
    // calculate the address of the next instruction ignoring jumps
    p1_pc = p2_pc + 4;    

    // But for the address to send to the iCache account for jumps
    if (reset)
        instr_address = 32'hffff0000;
    else if (p3_jump)
        instr_address = p3_jump_addr;
    else if (stall || p2_bubble)
        instr_address = p2_pc;
    else
        instr_address = p1_pc;
end

always @(posedge clock) begin
    if (!stall) begin
        p2_pc <= instr_address;
        p3_pc <= p2_pc;
        // $display("pc = %x", instr_address);
    end
end

endmodule