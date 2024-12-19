`timescale 1ns / 1ps

// Provide a read-only cache for the blitter to allow it to access memory in 
// bursts, then feed the result out the blitter pixel pipeline.

module blit_cache (
    input           clock,
    input           reset,

    // Blitter interface
    input      [31:0]    read_address,
    input                read_request,
    output     [7:0]     read_data,
    output reg           read_valid,
    output               read_stall,

    // Memory interface (other signals tied off - so we can only read burst from SDRAM))
    output reg [25:0]    mem_address,
    output reg           mem_request,
    input  [31:0]        mem_data,
    input                mem_valid,
    input                mem_ack,
    input                mem_complete,

    // Interface to pattern memory
    output [15:0]        pattern_address,
    input [31:0]         pattern_data
);

reg [31:0] data[7:0];
reg [25:5] cache_address;
reg [7:0]  cache_valid;
reg [1:0]  prev_addr_lsb;
reg [31:0] cache_data;
wire [31:0] rd;
reg [2:0]  write_ptr;

wire main_memory = read_address[31:26]== 7'h0;
wire patern_memory = read_address[31:16] == 16'hE100;
reg  prev_patern_memory;
reg  [1:0] fetch_state;
parameter IDLE    = 2'b00,
          REQUEST = 2'b01,
          LOADING = 2'b10;


assign pattern_address = patern_memory ? read_address[15:0] : 16'hx;

assign rd = prev_patern_memory ? pattern_data : cache_data;

assign read_data = (!read_valid)            ? 8'hbx     :
                   (prev_addr_lsb == 2'b00) ? rd[7:0]   :
                   (prev_addr_lsb == 2'b01) ? rd[15:8]  :
                   (prev_addr_lsb == 2'b10) ? rd[23:16] :
                   (prev_addr_lsb == 2'b11) ? rd[31:24] : 8'hx;

wire [2:0] f = read_address[4:2];
wire cv = cache_valid[f];
wire address_match = cache_address[25:5] == read_address[25:5];


wire tag_match = cv && address_match;
assign read_stall = !reset && read_request && main_memory && !tag_match;

always @(posedge clock) begin
    if (!read_stall) begin
        prev_addr_lsb <= read_address[1:0];
        prev_patern_memory <= patern_memory;
        cache_data <= data[read_address[4:2]];
        read_valid <= !reset && read_request;
    end

    case(fetch_state) 
        IDLE: begin
            if (read_stall) begin
                mem_request <= 1'b1;
                mem_address <= {read_address[25:5], 5'b00000};
                cache_address <= read_address[25:5];
                cache_valid <= 8'b0;
                write_ptr <= 3'h0;
                fetch_state <= REQUEST;
            end
        end
        
        REQUEST: begin
            if (mem_ack) begin
                mem_request <= 1'b0;
                fetch_state <= LOADING;
            end
        end

        LOADING: begin
            if (mem_valid) begin
                data[write_ptr] <= mem_data;
                cache_valid[write_ptr] <= 1'b1;
                write_ptr <= write_ptr + 1'b1;
            end

            if (mem_complete) begin
                fetch_state <= IDLE;
            end
        end
    endcase

    // ASSERTS
    if (mem_complete && fetch_state!=LOADING)
        $display("ERROR: mem_complete when not loading");
    if (mem_valid && fetch_state!=LOADING)
        $display("ERROR: mem_valid when not loading");
    if (mem_ack && fetch_state!=REQUEST)
        $display("ERROR: mem_ack when not request");

    if (reset) begin
        cache_valid <= 0;
        cache_address <= 0;
        mem_request <= 0;
        write_ptr <= 3'h0;
        fetch_state <= IDLE;
    end
end

endmodule