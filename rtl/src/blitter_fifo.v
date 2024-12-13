`timescale 1ns/1ns

module blitter_fifo #(
    parameter DEPTH = 256
) (
    input                   clock,
    input                   reset,
    
    // Write interface
    input [25:0]           wr_address,
    input [3:0]            wr_byte_en,
    input [31:0]           wr_data,
    input                  wr_valid,
    output                 wr_ready,
    
    // Read interface
    output reg [25:0]      rd_address,
    output reg [3:0]       rd_byte_en,
    output reg [31:0]      rd_data,
    output                 rd_valid,
    input                  rd_ready
);

    reg [61:0] fifo [0:DEPTH-1];  // 26 + 4 + 32 = 62 bits per entry
    reg [$clog2(DEPTH):0] wr_ptr, rd_ptr, prev_wr_ptr;
    wire empty = (prev_wr_ptr == rd_ptr); 
    wire full = (wr_ptr[($clog2(DEPTH)-1):0] == rd_ptr[($clog2(DEPTH)-1):0]) && 
                (wr_ptr[$clog2(DEPTH)] != rd_ptr[$clog2(DEPTH)]);
    
    assign rd_valid = !empty;
    assign wr_ready = !full;

    always @(posedge clock) begin
        if (reset) begin
            wr_ptr <= 0;
            rd_ptr <= 0;
        end else begin
            if (wr_valid && !full) begin
                fifo[wr_ptr[($clog2(DEPTH)-1):0]] <= {wr_address, wr_byte_en, wr_data};
                wr_ptr <= wr_ptr + 1'b1;
            end
            
            if (rd_ready && !empty)
                rd_ptr = rd_ptr + 1'b1;
            {rd_address, rd_byte_en, rd_data} <= fifo[rd_ptr[($clog2(DEPTH)-1):0]];

        end
        prev_wr_ptr <= wr_ptr;
    end
endmodule
