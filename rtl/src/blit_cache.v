`timescale 1ns / 1ps

// Provide a read-only cache for the blitter to allow it to access memory in 
// bursts, then feed the result out the blitter pixel pipeline.

module blit_cache (
    input           clock,
    input           reset,

    // Blitter interface
    input      [25:0]    read_address,
    input                read_request,
    output     [7:0]     read_data,
    output               read_stall,

    // Memory interface (other signals tied off - so we can only read burst from SDRAM))
    output reg [25:0]    mem_address,
    output reg           mem_request,
    input  [31:0]        mem_data,
    input                mem_valid,
    input                mem_ack,
    input                mem_complete
);

reg [31:0] data[7:0];
reg [25:5] cache_address;
reg        cache_valid;
reg [1:0]  prev_addr_lsb;
reg [31:0] cache_data;
reg [2:0]  write_ptr;
reg        prev_stall;

assign read_data = (prev_addr_lsb == 2'b00) ? cache_data[7:0] :
                   (prev_addr_lsb == 2'b01) ? cache_data[15:8] :
                   (prev_addr_lsb == 2'b10) ? cache_data[23:16] :
                   (prev_addr_lsb == 2'b11) ? cache_data[31:24] : 8'hx;

wire tag_match = cache_valid && cache_address[25:5] == read_address[25:5];
assign read_stall = !reset && read_request && !tag_match;

always @(posedge clock) begin
    prev_stall <= read_stall;
    if (!read_stall) begin
        cache_data <= data[read_address[4:2]];
        prev_addr_lsb <= read_address[1:0];
    end

    if (read_request && !tag_match && !prev_stall) begin
        mem_request <= 1'b1;
        mem_address <= {read_address[25:5], 5'b00000};
        write_ptr <= 3'h0;
    end

    if (mem_ack) begin
        mem_request <= 1'b0;
    end

    if (mem_valid) begin
        data[write_ptr] <= mem_data;
       write_ptr <= write_ptr + 1'b1;
    end

    if (mem_complete) begin
        cache_valid <= 1'b1;
        cache_address <= mem_address[25:5];
    end

    if (reset) begin
        cache_valid <= 0;
        cache_address <= 0;
        mem_request <= 0;
        write_ptr <= 3'h0;
    end
end

endmodule