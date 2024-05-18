`timescale 1ns/1ps

// ====================================================
//                 Address decoder
// ====================================================
// Get a memory access from the CPU and forward it on 
// to the different devices based on the address.
//
// Since the CPU is the only master to the IRAM/PRAM and HWREGS
// we never need to wait to pass a transaction on to them.
// The SDram however is shared between multiple masters, so 
// we need a register stage to hold the transaction before passing
// it on. 

module address_decoder(
    input            clock,
    input            reset,

    // CPU = bus master
    input             cpu_request,
    input             cpu_write,
    input      [31:0] cpu_address,
    input      [31:0] cpu_wdata,
    input      [3:0]  cpu_byte_en,
    output reg [31:0] cpu_rdata,
    output reg        cpu_valid,
    output reg        cpu_complete,

    // Instruction ram slave
    output reg         iram_request,
    output reg         iram_write,
    output reg [15:0]  iram_address,
    output reg [31:0]  iram_wdata,
    output reg [3:0]   iram_byte_en,
    input      [31:0]  iram_rdata,
    input              iram_valid,

    // SDRAM slave
    output reg          sdram_request,
    output reg          sdram_write,
    output reg  [25:0]  sdram_address,
    output reg  [31:0]  sdram_wdata,
    output reg  [3:0]   sdram_byte_en,
    input       [31:0]  sdram_rdata,
    input               sdram_valid,
    output reg          sdram_burst,
    input               sdram_complete,

    // Memory mapped regs slave
    output reg          hwregs_request,
    output reg          hwregs_write,
    output reg  [15:0]  hwregs_address,
    output reg  [31:0]  hwregs_wdata,
    output reg  [3:0]   hwregs_byte_en,
    input       [31:0]  hwregs_rdata,
    input               hwregs_valid,

    // Pattern memory slave
    output reg          pram_request,
    output reg          pram_write,
    output reg  [15:0]  pram_address,
    output reg  [31:0]  pram_wdata,
    output reg  [3:0]   pram_byte_en,
    input               pram_valid,
    input       [31:0]  pram_rdata
);

always @(*) begin
    iram_request   = 1'b0;
    iram_write     = 1'bx;
    iram_address   = 16'hx;
    iram_wdata     = 32'hx;
    iram_byte_en   = 4'hx;

    pram_request   = 1'b0;
    pram_write     = 1'bx;
    pram_address   = 16'hx;
    pram_wdata     = 32'hx;
    pram_byte_en   = 4'hx;

    hwregs_request = 1'b0;
    hwregs_write   = 1'bx;
    hwregs_address = 16'hx;
    hwregs_wdata   = 32'hx;
    hwregs_byte_en = 4'hx;

    if (cpu_request) begin
        if (cpu_address[31:26]==6'h0) begin
            // outputs to SDRAM handled in clocked section
        end else if (cpu_address[31:16]==16'hFFFF) begin
            iram_request   = 1'b1;
            iram_write     = cpu_write;
            iram_address   = cpu_address[15:0];
            iram_wdata     = cpu_wdata;
            iram_byte_en   = cpu_byte_en;
        end else if (cpu_address[31:16]==16'hE000) begin
            hwregs_request = 1'b1;
            hwregs_write   = cpu_write;
            hwregs_address = cpu_address[15:0];
            hwregs_wdata   = cpu_wdata;
            hwregs_byte_en = cpu_byte_en;
        end else if (cpu_address[31:16]==16'hE100) begin
            pram_request   = 1'b1;
            pram_write     = cpu_write;
            pram_address   = cpu_address[15:0];
            pram_wdata     = cpu_wdata;
            pram_byte_en   = cpu_byte_en;
        end else begin
            // TODO = need to implement bus error exceptions
            $display("TIME %t - invalid address %x",$time,cpu_address);
        end
    end


    cpu_rdata      = sdram_rdata | iram_rdata | hwregs_rdata | pram_rdata;
    cpu_valid      = sdram_valid | iram_valid | hwregs_valid | pram_valid;
    cpu_complete   = sdram_complete | iram_valid | hwregs_valid | pram_valid;
end

// register CPU transactions destined for the SDRAM, and hold them until complete.
// Transactions are not allowed to overlap, but we can have the case where one
// transaction starts immediately as one finishes.

always @(posedge clock) begin
    if (reset || (sdram_complete && !cpu_request)) begin
        sdram_request = 1'b0;
        sdram_write   = 1'bx;
        sdram_address = 26'hx;
        sdram_wdata   = 32'hx;
        sdram_byte_en = 4'hx;
        sdram_burst   = 1'bx;
    end else if (cpu_request && cpu_address[31:26]==7'd0) begin
        sdram_request = 1'b1;
        sdram_write   = cpu_write;
        sdram_address = cpu_address[25:0];
        sdram_wdata   = cpu_wdata;
        sdram_byte_en = cpu_byte_en;        
        sdram_burst   = 1'b0;   // CPU currently never requests bursts
    end
end

endmodule