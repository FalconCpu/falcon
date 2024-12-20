`timescale 1ns/1ps

// The handshake protocol is
// To initiate a request a master sets its write/address/byte_en/wdata and asserts request
// The arbiter then pulses ack when it has accepted the request. This may be on the same cycle as it was asserted.
// The master then deasserts its request.
// For read transactions the data arrives some number of cycles later.

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
    output          dcache_ack,
    output  [31:0]  dcache_rdata,
    output          dcache_valid,
    output          dcache_complete,

    // VGA = bus master 2  (read only)
    input           vga_request,
    input   [25:0]  vga_address,
    output  [31:0]  vga_rdata,
    output          vga_ack,
    output          vga_valid,
    output          vga_complete,

    // BLITW = blitter read 3  (write only)
    input           blitw_request,
    input   [25:0]  blitw_address,
    input   [31:0]  blitw_wdata,
    input   [3:0]   blitw_byte_en,
    output          blitw_ack,

    // BLITR = blitter read 4  (read only)
    input           blitr_request,
    input   [25:0]  blitr_address,
    output  [31:0]  blitr_rdata,
    output          blitr_ack,
    output          blitr_valid,
    output          blitr_complete,

    // ICACHE = instruction cache 5 (read only)
    input           icache_request,
    input  [25:0]   icache_address,
    output [31:0]   icache_rdata,
    output          icache_ack,
    output          icache_valid,
    output          icache_complete,


    // SDRAM slave
    output reg          sdram_request,
    output reg          sdram_write,
    output      [3:0]   sdram_master,
    output reg  [25:0]  sdram_address,
    output reg  [31:0]  sdram_wdata,
    output reg  [3:0]   sdram_byte_en,
    input       [31:0]  sdram_rdata,
    input       [3:0]   sdram_valid,
    output reg          sdram_burst,
    input               sdram_ready,
    input       [3:0]   sdram_complete
);

reg [3:0]  this_master, next_master;

assign dcache_valid    = sdram_valid == 3'h1;
assign dcache_complete = sdram_complete == 3'h1;
assign dcache_rdata    = dcache_valid ? sdram_rdata : 32'b0;
assign dcache_ack      = sdram_ready && next_master==4'h1;

assign vga_valid    = sdram_valid == 3'h2;
assign vga_complete = sdram_complete == 3'h2;
assign vga_rdata    = vga_valid ? sdram_rdata : 32'bx;
assign vga_ack      = sdram_ready && next_master==4'h2;

assign blitw_ack      = sdram_ready && next_master==4'h3;

assign blitr_valid    = sdram_valid == 3'h4;
assign blitr_complete = sdram_complete == 3'h4;
assign blitr_rdata    = blitr_valid ? sdram_rdata : 32'bx;
assign blitr_ack      = sdram_ready && next_master==4'h4;

assign icache_valid    = sdram_valid == 3'h5;
assign icache_complete = sdram_complete == 3'h5;
assign icache_rdata    = sdram_valid ? sdram_rdata : 32'bx;
assign icache_ack      = sdram_ready && next_master==4'h5;

assign sdram_master = this_master;
reg prev_match;

always @(*) begin
    // determine the new bus master
    if (sdram_ready) begin
        if (dcache_request)
            next_master = 4'b1;
        else if (vga_request)
            next_master = 4'd2;
        else if (blitw_request)
            next_master = 4'd3;
        else if (blitr_request)
            next_master = 4'd4;
        else if (icache_request)
            next_master = 4'd5;
        else
            next_master = 4'b0;
    end else
        next_master = this_master;
end

reg match;
always @(posedge clock) begin
    this_master <= next_master;

    if (reset || sdram_ready) begin
        sdram_request <= next_master != 4'b0;
        case (next_master)
            4'h1: begin
                sdram_write   <= dcache_write;
                sdram_address <= dcache_address;
                sdram_wdata   <= dcache_wdata;
                sdram_byte_en <= dcache_byte_en;
                sdram_burst   <= dcache_burst;
            end

            4'h2: begin
                sdram_write   <= 1'b0;
                sdram_address <= vga_address;
                sdram_wdata   <= 26'bx;
                sdram_byte_en <= 4'b0;
                sdram_burst   <= 1'h1;
            end

            4'h3: begin  // Blitter does single writes
                sdram_write   <= 1'b1;
                sdram_address <= blitw_address;
                sdram_wdata   <= blitw_wdata;
                sdram_byte_en <= blitw_byte_en;
                sdram_burst   <= 1'h0;
            end

            4'h4: begin  // Blitter always does burst reads
                sdram_write   <= 1'b0;
                sdram_address <= blitr_address;
                sdram_wdata   <= 26'bx;
                sdram_byte_en <= 4'b0;
                sdram_burst   <= 1'h1;
            end

            4'h5: begin  // ICache always does burst reads
                sdram_write   <= 1'b0;
                sdram_address <= icache_address;
                sdram_wdata   <= 26'bx;
                sdram_byte_en <= 4'b0;
                sdram_burst   <= 1'h1;
            end

            default: begin
                sdram_write   <= 1'bx;
                sdram_address <= 26'hx;
                sdram_wdata   <= 26'bx;
                sdram_byte_en <= 4'hx;
                sdram_burst   <= 1'hx;
            end
        endcase
    end

    if (sdram_request && sdram_ready)
        if (sdram_write)
            $display("TIME:%t WRITE MASTER=%d A=%08x D=%08x %x",$time, sdram_master, sdram_address, sdram_wdata, sdram_byte_en);
        else if (sdram_master!=2)
            $display("TIME:%t READ  MASTER=%d A=%08x",$time, sdram_master, sdram_address);
    if (sdram_valid!=0 && sdram_valid!=2)
        $display("TIME:%t READ  MASTER=%d D=%08x", $time, sdram_valid, sdram_rdata);
end

endmodule