`timescale 1ns/1ns

// Basically a pass through for now.
// Later this will become a cache.
//
// The CPU only pulses requests for a single cycle - but the SDRAM arbiter
// requires the masters to assert request for the entire transaction. So we
// need to latch the cpu request.


module data_cache (
    input            clock,
    input            reset,

    // Connections to the CPU
    input            cpu_request,
    input [25:0]     cpu_address,
    input            cpu_write,
    input [3:0]      cpu_wstrb,
    input [31:0]     cpu_wdata,
    output [31:0]    cpu_rdata,
    output           cpu_valid,

    // connections to the sdram arbiter
    output reg        sdram_request,
    output reg        sdram_write,
    output reg [25:0] sdram_address,
    output reg        sdram_burst,
    output reg [31:0] sdram_wdata,
    output reg [3:0]  sdram_byte_en,
    input      [31:0] sdram_rdata,
    input             sdram_valid,
    input             sdram_complete,
    input             sdram_ack
);

assign cpu_rdata = sdram_valid  ? sdram_rdata : 32'b0;
assign cpu_valid = sdram_valid || (sdram_write && sdram_ack);
wire   sdram_free = !sdram_request || sdram_ack;

always @(posedge clock) begin
    if (cpu_request && sdram_free) begin   
        sdram_request <= 1'b1;
        sdram_write   <= cpu_write;
        sdram_address <= cpu_address;
        sdram_wdata   <= cpu_wdata;
        sdram_byte_en <= cpu_wstrb;
        sdram_burst   <= 1'b0;   // CPU currently never requests bursts
    end else if (reset || sdram_ack) begin
        sdram_request <= 1'b0;
        sdram_write   <= 1'bx;
        sdram_address <= 26'hx;
        sdram_wdata   <= 32'hx;
        sdram_byte_en <= 4'hx;
        sdram_burst   <= 1'bx;
    end 
end

endmodule

