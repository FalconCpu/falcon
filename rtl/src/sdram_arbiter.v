`timescale 1ns/1ps

// The handshake protocol is
// To initiate a request a master sets its write/address/byte_en/wdata and asserts request
// The arbiter forwards this on to the SDRAM controller
// The sdram controller pulses valid for each 32 bit word of the data
// The sdram controller pulses complete to indicate the transaction has been completed, and the master deasserts request.clock
// Note - the final word of data may arrives one cycle after the complete signal 



module sdram_arbiter(
    input            clock,
    input            reset,

    // CPU = bus master 1
    input           cpu_request,
    input           cpu_write,
    input   [25:0]  cpu_address,
    input           cpu_burst,
    input   [31:0]  cpu_wdata,
    input   [3:0]   cpu_byte_en,
    output  [31:0]  cpu_rdata,
    output          cpu_valid,
    output          cpu_complete,

    // VGA = bus master 2  (read only)
    input           vga_request,
    input   [25:0]  vga_address,
    output  [31:0]  vga_rdata,
    output          vga_valid,
    output          vga_complete,

    // GPU = bus master 3
    input           gpu_request,
    input           gpu_write,
    input   [25:0]  gpu_address,
    input   [31:0]  gpu_wdata,
    input           gpu_burst,
    input   [3:0]   gpu_byte_en,
    output  [31:0]  gpu_rdata,
    output          gpu_valid,
    output          gpu_complete,

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

assign sdram_request = next_master!=2'h0;

assign cpu_valid    = sdram_valid[0];
assign cpu_complete = sdram_complete[0];
assign cpu_rdata    = cpu_valid ? sdram_rdata : 32'b0;

assign vga_valid    = sdram_valid[1];
assign vga_complete = sdram_complete[1];
assign vga_rdata    = vga_valid ? sdram_rdata : 32'bx;

assign gpu_valid    = sdram_valid[2];
assign gpu_complete = sdram_complete[2];
assign gpu_rdata    = gpu_valid ? sdram_rdata : 32'bx;

assign sdram_master = next_master;

always @(*) begin
    // determine the new bus master
    next_master = this_master;
    if (sdram_complete || reset) 
        next_master = 4'b0;
    else if (next_master==3'b0 && cpu_request)
        next_master = 4'b1;
    else if (next_master==3'b0 && vga_request)
        next_master = 4'd2;
    else if (next_master==3'b0 && gpu_request)
        next_master = 4'd4;

    // get the signals from the new master
    case (next_master)
        4'h1: begin
            sdram_write = cpu_write;
            sdram_address = cpu_address;
            sdram_wdata = cpu_wdata;
            sdram_byte_en = cpu_byte_en;
            sdram_burst = cpu_burst;
        end

        4'h2: begin
            sdram_write = 1'b0;
            sdram_address = vga_address;
            sdram_wdata = 26'bx;;
            sdram_byte_en = 4'b0;
            sdram_burst   = 1'h1;
        end

        4'h4: begin
            sdram_write = gpu_write;
            sdram_address = gpu_address;
            sdram_wdata = gpu_wdata;
            sdram_byte_en = gpu_byte_en;
            sdram_burst = gpu_burst;
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