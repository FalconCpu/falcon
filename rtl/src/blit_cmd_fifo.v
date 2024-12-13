`timescale 1ns/1ns

module blitter_cmd_fifo (
    input                   clock,
    input                   reset,
    
    // Interface to hwregs
    input [103:0]          blit_cmd,
    input                  blit_start,
    output [7:0]           blit_slots_free,
    
    // Read interface
    output reg [103:0]     cmd_cmd,
    output                 cmd_valid,
    input                  cmd_next
);

    reg [103:0] fifo [0:255];
    reg [7:0]   wr_ptr, rd_ptr, prev_wr_ptr;
    assign blit_slots_free = rd_ptr - prev_wr_ptr - 1'b1;
    assign cmd_valid = rd_ptr != prev_wr_ptr;

always @(posedge clock) begin
    if (blit_start) begin
        fifo[wr_ptr] <= blit_cmd;
        wr_ptr <= wr_ptr + 1'b1;
    end
       
    cmd_cmd <= fifo[rd_ptr];
    if (cmd_next && cmd_valid)
        rd_ptr = rd_ptr + 1'b1;

    if (reset) begin
        wr_ptr <= 0;
        rd_ptr <= 0;
    end 
    prev_wr_ptr <= wr_ptr;
end
endmodule
