`timescale 1ns/1ps

module bus_arbiter(
    input            clock,
    input            reset,

    // CPU = bus master 1
    input           cpu_request,
    input           cpu_write,
    input   [31:0]  cpu_address,
    input   [31:0]  cpu_wdata,
    input   [3:0]   cpu_byte_en,
    output  [31:0]  cpu_rdata,
    output          cpu_ack,

    // Instruction ram slave
    output reg      iram_request,
    output          iram_write,
    output  [15:0]  iram_address,
    output  [31:0]  iram_wdata,
    output  [3:0]   iram_byte_en,
    input   [31:0]  iram_rdata,
    input           iram_ack,

    // SDRAM slave
    output reg      sdram_request,
    output          sdram_write,
    output  [25:0]  sdram_address,
    output  [31:0]  sdram_wdata,
    output  [3:0]   sdram_byte_en,
    input   [31:0]  sdram_rdata,
    input           sdram_ack,

    // Memory mapped regs slave
    output reg      hwregs_request,
    output          hwregs_write,
    output  [15:0]  hwregs_address,
    output  [31:0]  hwregs_wdata,
    output  [3:0]   hwregs_byte_en,
    input   [31:0]  hwregs_rdata,
    input           hwregs_ack

);

reg [1:0]  this_bus_master, next_bus_master;

reg        request;
reg        write;
reg [31:0] address;
reg [31:0] wdata;
reg [3:0]  byte_en;
reg [31:0] rdata;

assign iram_write   = iram_request ? write : 1'bx;
assign iram_address = iram_request ? address[15:0] : 16'bx;
assign iram_wdata   = iram_request ? wdata : 32'bx;
assign iram_byte_en = iram_request ? byte_en : 4'bx;

assign sdram_write   = sdram_request ? write : 1'bx;
assign sdram_address = sdram_request ? address[25:0] : 16'bx;
assign sdram_wdata   = sdram_request ? wdata : 32'bx;
assign sdram_byte_en = sdram_request ? byte_en : 4'bx;

assign hwregs_write   = hwregs_request ? write : 1'bx;
assign hwregs_address = hwregs_request ? address[15:0] : 16'bx;
assign hwregs_wdata   = hwregs_request ? wdata : 32'bx;
assign hwregs_byte_en = hwregs_request ? byte_en : 4'bx;

// combine the rdata and ack signals from all the slaves
// Do this with a simple OR gate - requires slaves to keep their output at 
// zero when not selected
assign cpu_rdata = iram_rdata | sdram_rdata | hwregs_rdata;
assign cpu_ack   = iram_ack | sdram_ack | hwregs_ack;


always @(*) begin
    

    // determine the new bus master
    next_bus_master = this_bus_master;
    if (cpu_ack || reset) 
        next_bus_master = 2'b0;
    if (next_bus_master==2'b0 && cpu_request)
        next_bus_master = 2'b1;
    request = next_bus_master != 2'h0;

    // get the signals from the new master
    case (next_bus_master)
        2'h1: begin
            write = cpu_write;
            address = cpu_address;
            wdata = cpu_wdata;
            byte_en = cpu_byte_en;
        end

        default: begin
            write = 1'b0;
            address = 32'h0;
            wdata = 32'h0;
            byte_en = 4'h0;
        end
    endcase
    
    // address decode to determine the target
    iram_request = 1'b0;
    sdram_request = 1'b0;
    hwregs_request = 1'b0;
    if (request && address[31:26]==6'h0) 
        sdram_request = 1'b1;
    else if (request && address[31:16]==16'hFFFF) 
        iram_request = 1'b1;
    else if (request && address[31:16]==16'hE000) 
        hwregs_request = 1'b1;
end

always @(posedge clock) begin
    this_bus_master <= next_bus_master;
end

endmodule