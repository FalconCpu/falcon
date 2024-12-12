`timescale 1ns/1ps

// The handshake protocol is
// To initiate a request a master sets its write/address/byte_en/wdata and asserts request
// The arbiter forwards this on to the SDRAM controller
// The sdram controller pulses valid for each 32 bit word of the data
// The sdram controller pulses complete to indicate the transaction has been completed, and the master deasserts request.
// Note - on a read the final word of data may arrives one cycle after the complete signal 



module sdram_arbiter(
    input            clock,
    input            reset,

    // CPU = bus master 1
    input           dcache_request,
    input           dcache_write,
    input   [25:0]  dcache_address,
    input           dcache_burst,
    input   [31:0]  dcache_wdata,
    input   [3:0]   dcache_byte_en,
    output  [31:0]  dcache_rdata,
    output          dcache_valid,
    output          dcache_complete,

    // VGA = bus master 2  (read only)
    input           vga_request,
    input   [25:0]  vga_address,
    output  [31:0]  vga_rdata,
    output          vga_valid,
    output          vga_complete,

    // VGA = blitter read 3  (read only)
    input           blitw_request,
    input   [25:0]  blitw_address,
    input   [31:0]  blitw_wdata,
    input   [3:0]   blitw_byte_en,
    output          blitw_complete,

    // VGA = blitter read 4  (read only)
    input           blitr_request,
    input   [25:0]  blitr_address,
    output  [31:0]  blitr_rdata,
    output          blitr_valid,
    output          blitr_complete,


    // SDRAM slave
    output              sdram_request,
    output reg          sdram_write,
    output      [3:0]   sdram_master,
    output reg  [25:0]  sdram_address,
    output reg  [31:0]  sdram_wdata,
    output reg  [3:0]   sdram_byte_en,
    input       [31:0]  sdram_rdata,
    input       [3:0]   sdram_valid,
    output reg          sdram_burst,
    input       [3:0]   sdram_complete
);

reg [3:0]  this_master, next_master;

assign sdram_request = next_master!=3'h0;

assign dcache_valid    = sdram_valid == 3'h1;
assign dcache_complete = sdram_complete == 3'h1;
assign dcache_rdata    = dcache_valid ? sdram_rdata : 32'b0;

assign vga_valid    = sdram_valid == 3'h2;
assign vga_complete = sdram_complete == 3'h2;
assign vga_rdata    = vga_valid ? sdram_rdata : 32'bx;

assign blitw_complete = sdram_complete == 3'h3;

assign blitr_valid    = sdram_valid == 3'h4;
assign blitr_complete = sdram_complete == 3'h4;
assign blitr_rdata    = blitr_valid ? sdram_rdata : 32'bx;

assign sdram_master = next_master;

always @(*) begin
    // determine the new bus master
    next_master = this_master;
    if (sdram_complete || reset) 
        next_master = 4'b0;
    else if (next_master==3'b0 && dcache_request)
        next_master = 4'b1;
    else if (next_master==3'b0 && vga_request)
        next_master = 4'd2;
    else if (next_master==3'b0 && blitr_request)
        next_master = 4'd4;
    else if (next_master==3'b0 && blitw_request)
        next_master = 4'd3;
        

    // get the signals from the new master
    case (next_master)
        4'h1: begin
            sdram_write = dcache_write;
            sdram_address = dcache_address;
            sdram_wdata = dcache_wdata;
            sdram_byte_en = dcache_byte_en;
            sdram_burst = dcache_burst;
        end

        4'h2: begin
            sdram_write = 1'b0;
            sdram_address = vga_address;
            sdram_wdata = 26'bx;;
            sdram_byte_en = 4'b0;
            sdram_burst   = 1'h1;
        end

        4'h3: begin  // Blitter does single writes
            sdram_write   = 1'b1;
            sdram_address = blitw_address;
            sdram_wdata   = blitw_wdata;
            sdram_byte_en = blitw_byte_en;
            sdram_burst   = 1'h0;
        end

        4'h4: begin  // Blitter always does burst reads
            sdram_write   = 1'b0;
            sdram_address = blitr_address;
            sdram_wdata   = 26'bx;;
            sdram_byte_en = 4'b0;
            sdram_burst   = 1'h1;
        end


        default: begin
            sdram_write = 1'bx;
            sdram_address = 26'hx;
            sdram_wdata = 32'hx;
            sdram_byte_en = 4'hx;
            sdram_burst   = 1'hx;
        end
    endcase
end

always @(posedge clock) begin
    this_master <= next_master;
end

endmodule