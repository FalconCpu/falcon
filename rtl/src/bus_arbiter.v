`timescale 1ns/1ps

module bus_arbiter(
    input            clock,
    input            reset,

    // CPU = bus master 1
    input           cpu_request,
    input           cpu_write,
    input   [31:0]  cpu_address,
    input           cpu_burst,
    input   [31:0]  cpu_wdata,
    input   [3:0]   cpu_byte_en,
    output  [31:0]  cpu_rdata,
    output          cpu_valid,
    output          cpu_complete,

    // VGA = bus master 2  (read only)
    input           vga_request,
    input   [31:0]  vga_address,
    output  [31:0]  vga_rdata,
    output          vga_valid,
    output          vga_complete,

    // GPU = bus master 3
    input           gpu_request,
    input           gpu_write,
    input   [31:0]  gpu_address,
    input   [31:0]  gpu_wdata,
    input           gpu_burst,
    input   [3:0]   gpu_byte_en,
    output  [31:0]  gpu_rdata,
    output          gpu_valid,
    output          gpu_complete,


    // Instruction ram slave
    output             iram_request,
    output reg         iram_write,
    output reg [15:0]  iram_address,
    output reg [31:0]  iram_wdata,
    output reg [3:0]   iram_byte_en,
    input   [31:0]  iram_rdata,
    input           iram_valid,

    // SDRAM slave
    output              sdram_request,
    output reg          sdram_write,
    output reg  [25:0]  sdram_address,
    output reg  [31:0]  sdram_wdata,
    output reg  [3:0]   sdram_byte_en,
    input       [31:0]  sdram_rdata,
    input               sdram_valid,
    output reg          sdram_burst,
    input               sdram_complete,

    // Memory mapped regs slave
    output              hwregs_request,
    output reg          hwregs_write,
    output reg  [15:0]  hwregs_address,
    output reg  [31:0]  hwregs_wdata,
    output reg  [3:0]   hwregs_byte_en,
    input   [31:0]      hwregs_rdata,
    input               hwregs_valid,

    // Pattern memory mapped regs slave
    output              pram_request,
    output reg          pram_write,
    output reg  [15:0]  pram_address,
    output reg  [31:0]  pram_wdata,
    output reg  [3:0]   pram_byte_en,
    input               pram_valid,
    input       [31:0]  pram_rdata


);

reg [1:0]  this_master, next_master;
reg [2:0]  this_slave, next_slave;



reg        write;
reg [31:0] address;
reg [31:0] wdata;
reg        burst;
reg [3:0]  byte_en;

// combine the rdata and ack signals from all the slaves
// Do this with a simple OR gate - requires slaves to keep their output at 
// zero when not selected
wire   valid     = iram_valid | sdram_valid | hwregs_valid | pram_valid;
wire   complete  = iram_valid | sdram_complete | hwregs_valid | pram_valid;
wire [31:0] rdata = iram_rdata | sdram_rdata | hwregs_rdata | pram_rdata;

assign cpu_valid   = valid && this_master==1;
assign cpu_complete = complete && this_master==1;
assign cpu_rdata = cpu_valid ? rdata : 32'bx;

assign vga_valid   = valid && this_master==2;
assign vga_complete = complete && this_master==2;
assign vga_rdata = vga_valid ? rdata : 32'bx;

assign gpu_valid   = valid && this_master==3;
assign gpu_complete = complete && this_master==3;
assign gpu_rdata = gpu_valid ? rdata : 32'bx;


reg hwregs_request_reg;
reg pram_request_reg;
reg sdram_request_reg;
reg iram_request_reg;
assign hwregs_request = hwregs_request_reg && !complete;
assign pram_request = pram_request_reg && !complete;
assign sdram_request = sdram_request_reg && !complete;
assign iram_request = iram_request_reg && !complete;


always @(*) begin
    // determine the new bus master
    next_master = this_master;
    if (complete || reset) 
        next_master = 2'b0;
    
    if (next_master==2'b0 && cpu_request && this_master!=2'd2)
        next_master = 2'b1;
    else if (next_master==2'b0 && vga_request && this_master!=2'h2)
        next_master = 2'd2;
    else if (next_master==2'b0 && gpu_request && this_master!=2'h3)
        next_master = 2'd3;

    // get the signals from the new master
    case (next_master)
        2'h1: begin
            write = cpu_write;
            address = cpu_address;
            wdata = cpu_wdata;
            byte_en = cpu_byte_en;
            burst = cpu_burst;
        end

        2'h2: begin
            write = 1'b0;
            address = vga_address;
            wdata = 32'bx;;
            byte_en = 4'b0;
            burst   = 1'h1;
        end

        2'h3: begin
            write = gpu_write;
            address = gpu_address;
            wdata = gpu_wdata;
            byte_en = gpu_byte_en;
            burst = gpu_burst;
        end

        default: begin
            write = 1'b0;
            address = 32'h0;
            wdata = 32'h0;
            byte_en = 4'h0;
            burst   = 1'h1;
        end
    endcase
    
    // address decode to determine the target
    if (next_master==0)
        next_slave = 3'h0;
    else if (address[31:26]==6'h0) 
        next_slave = 3'h1;
    else if (address[31:16]==16'hFFFF) 
        next_slave = 3'h2;
    else if (address[31:16]==16'hE000) 
        next_slave = 3'h3;
    else if (address[31:16]==16'hE100) 
        next_slave = 3'h4;
    else begin
        $display("Time %t: ERROR INVALID ADDRESS %x", $abstime, address);
        next_slave = 3'h0;
    end
end

always @(posedge clock) begin
    this_master <= next_master;
    this_slave  <= next_slave;

    if (next_slave==3'h1) begin
        sdram_request_reg <= 1'h1;
        sdram_write   <= write;
        sdram_address <= address[25:0];
        sdram_wdata   <= wdata;
        sdram_byte_en <= byte_en;
        sdram_burst   <= burst;
    end else begin
        sdram_request_reg <= 1'h0;
        sdram_write   <= 1'hx;
        sdram_address <= 26'hx;
        sdram_wdata   <= 32'hx;
        sdram_byte_en <= 4'hx;
        sdram_burst   <= 1'hx;
    end

    if (next_slave==3'h2) begin
        iram_request_reg <= 1'b1;
        iram_write   <= write;
        iram_address <= address[15:0];
        iram_wdata   <= wdata;
        iram_byte_en <= byte_en;
    end else begin
        iram_request_reg <= 1'b0;
        iram_write   <= 1'bx;
        iram_address <= 16'hx;
        iram_wdata   <= 32'hx;
        iram_byte_en <= 4'hx;
    end
    
    if (next_slave==3'h3) begin
        hwregs_request_reg <= 1'h1;
        hwregs_write   <= write;
        hwregs_address <= address[15:0];
        hwregs_wdata   <= wdata;
        hwregs_byte_en <= byte_en;
    end else begin
        hwregs_request_reg <= 1'h0;
        hwregs_write   <= 1'hx;
        hwregs_address <= 16'hx;
        hwregs_wdata   <= 32'hx;
        hwregs_byte_en <= 4'hx;
    end

    if (next_slave==3'h4) begin
        pram_request_reg   <= 1'h1;
        pram_write     <= write;
        pram_address   <= address[15:0];
        pram_wdata     <= wdata;
        pram_byte_en   <= byte_en;
    end else begin
        pram_request_reg   <= 1'h0;
        pram_write     <= 1'hx;
        pram_address   <= 16'hx;
        pram_wdata     <= 32'hx;
        pram_byte_en   <= 4'hx;
    end

    // asserts for multiple slaves signaling at once
    if (sdram_rdata!=0 && sdram_rdata!=rdata)
        $display("TIME %t Data corruption on sdram_rdata",$time);
    if (pram_rdata!=0 && pram_rdata!=rdata)
        $display("TIME %t Data corruption on pram_rdata",$time);
    if (iram_rdata!=0 && iram_rdata!=rdata)
        $display("TIME %t Data corruption on pram_rdata",$time);
    if (hwregs_rdata!=0 && hwregs_rdata!=rdata)
        $display("TIME %t Data corruption on hwregs_rdata",$time);

end

endmodule