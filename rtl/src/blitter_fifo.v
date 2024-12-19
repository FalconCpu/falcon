`timescale 1ns/1ns

module blitter_fifo (
    input                   clock,
    input                   reset,
    
    // Write interface
    input [25:0]           wr_address,
    input [3:0]            wr_byte_en,
    input [31:0]           wr_data,
    input                  wr_valid,
    output reg             wr_full,
    
    // Read interface
    output reg [25:0]      rd_address,
    output reg [3:0]       rd_byte_en,
    output reg [31:0]      rd_data,
    output                 rd_valid,
    input                  rd_ready
);

    reg [61:0] fifo [0:255];  // 26 + 4 + 32 = 62 bits per entry
    reg [7:0] wr_ptr, prev_wr_ptr, rd_ptr;

    wire empty = (prev_wr_ptr == rd_ptr); 
    assign rd_valid = !empty;
    wire [7:0] inc_rd_ptr = (rd_ready && !empty) ? rd_ptr + 1'b1 : rd_ptr;


    always @(posedge clock) begin
        if (reset) begin
            wr_ptr <= 0;
            rd_ptr <= 0;
        end else begin
            if (wr_valid) begin
                fifo[wr_ptr[7:0]] <= {wr_address, wr_byte_en, wr_data};
                wr_ptr <= wr_ptr + 1'b1;
            end
            
            rd_ptr <= inc_rd_ptr;
            {rd_address, rd_byte_en, rd_data} <= fifo[inc_rd_ptr[7:0]];
        end
        prev_wr_ptr <= wr_ptr;
        wr_full <= (wr_ptr!=rd_ptr) && ((rd_ptr - wr_ptr) < 16);
    end
endmodule
