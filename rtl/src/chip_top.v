`timescale 1ns/1ps

module chip_top (

	//////////// Audio //////////
input 		          		AUD_ADCDAT,
inout 		          		AUD_ADCLRCK,
inout 		          		AUD_BCLK,
output		          		AUD_DACDAT,
inout 		          		AUD_DACLRCK,
output		          		AUD_XCK,

//////////// CLOCK //////////
input 		          		CLOCK2_50,
input 		          		CLOCK3_50,
input 		          		CLOCK4_50,
input 		          		CLOCK_50,

//////////// SDRAM //////////
output		    [12:0]		DRAM_ADDR,
output		     [1:0]		DRAM_BA,
output		          		DRAM_CAS_N,
output		          		DRAM_CKE,
output		          		DRAM_CLK,
output		          		DRAM_CS_N,
inout 		    [15:0]		DRAM_DQ,
output		          		DRAM_LDQM,
output		          		DRAM_RAS_N,
output		          		DRAM_UDQM,
output		          		DRAM_WE_N,

//////////// I2C for Audio and Video-In //////////
output		          		FPGA_I2C_SCLK,
inout 		          		FPGA_I2C_SDAT,

//////////// SEG7 //////////
output		     [6:0]		HEX0,
output		     [6:0]		HEX1,
output		     [6:0]		HEX2,
output		     [6:0]		HEX3,
output		     [6:0]		HEX4,
output		     [6:0]		HEX5,

//////////// KEY //////////
input 		     [3:0]		KEY,

//////////// LED //////////
output		     [9:0]		LEDR,

//////////// PS2 //////////
inout 		          		PS2_CLK,
inout 		          		PS2_CLK2,
inout 		          		PS2_DAT,
inout 		          		PS2_DAT2,

//////////// SW //////////
input 		     [9:0]		SW,

//////////// VGA //////////
output		          		VGA_BLANK_N,
output		     [7:0]		VGA_B,
output		          		VGA_CLK,
output		     [7:0]		VGA_G,
output		          		VGA_HS,
output		     [7:0]		VGA_R,
output		          		VGA_SYNC_N,
output		          		VGA_VS,

//////////// GPIO_0, GPIO_0 connect to GPIO Default //////////
inout 		    [35:0]		GPIO_0,

//////////// GPIO_1, GPIO_1 connect to GPIO Default //////////
inout 		    [35:0]		GPIO_1
);

// signals driven by instruction ram
wire [31:0]    instr_address;
wire [31:0]    instr_data;

// signals driven by PLL
wire           clock;
wire           locked;

// memory busses:-
wire           cpu_request;
wire           cpu_write;
wire [31:0]    cpu_address;
wire [31:0]    cpu_wdata;
wire [3:0]     cpu_byte_en;
wire [31:0]    cpu_rdata;
wire           cpu_ack;

wire           iram_request;
wire           iram_write;
wire [15:0]    iram_address;
wire [31:0]    iram_wdata;
wire [3:0]     iram_byte_en;
wire [31:0]    iram_rdata;
wire           iram_ack;

wire           sdram_request;
wire           sdram_write;
wire [25:0]    sdram_address;
wire [31:0]    sdram_wdata;
wire [3:0]     sdram_byte_en;
wire [31:0]    sdram_rdata;
wire           sdram_ack;

wire           hwregs_request;
wire           hwregs_write;
wire [15:0]    hwregs_address;
wire [31:0]    hwregs_wdata;
wire [3:0]     hwregs_byte_en;
wire [31:0]    hwregs_rdata;
wire           hwregs_ack;

// signals driven by hwregs
wire [23:0]    seven_segment;
wire [7:0]     seven_segment_brightness;



wire reset = !locked;

pll  pll_inst (
    .refclk(CLOCK_50),
    .rst(1'b0),
    .outclk_0(clock),
    .outclk_1(DRAM_CLK),
    .locked(locked)
  );

cpu  cpu_inst (
    .clock(clock),
    .reset(reset),
    .cpu_request(cpu_request),
    .cpu_write(cpu_write),
    .cpu_address(cpu_address),
    .cpu_wdata(cpu_wdata),
    .cpu_byte_en(cpu_byte_en),
    .cpu_rdata(cpu_rdata),
    .cpu_ack(cpu_ack),
    .instr_address(instr_address),
    .instr_data(instr_data)
  );

bus_arbiter  bus_arbiter_inst (
   .clock(clock),
   .reset(reset),
   .cpu_request(cpu_request),
   .cpu_write(cpu_write),
   .cpu_address(cpu_address),
   .cpu_wdata(cpu_wdata),
   .cpu_byte_en(cpu_byte_en),
   .cpu_rdata(cpu_rdata),
   .cpu_ack(cpu_ack),
   .iram_request(iram_request),
   .iram_write(iram_write),
   .iram_address(iram_address),
   .iram_wdata(iram_wdata),
   .iram_byte_en(iram_byte_en),
   .iram_rdata(iram_rdata),
   .iram_ack(iram_ack),
   .sdram_request(sdram_request),
   .sdram_write(sdram_write),
   .sdram_address(sdram_address),
   .sdram_wdata(sdram_wdata),
   .sdram_byte_en(sdram_byte_en),
   .sdram_rdata(sdram_rdata),
   .sdram_ack(sdram_ack),
   .hwregs_request(hwregs_request),
   .hwregs_write(hwregs_write),
   .hwregs_address(hwregs_address),
   .hwregs_wdata(hwregs_wdata),
   .hwregs_byte_en(hwregs_byte_en),
   .hwregs_rdata(hwregs_rdata),
   .hwregs_ack(hwregs_ack)
 );

instruction_ram  instruction_ram_inst (
    .clock(clock),
    .iram_request(iram_request),
    .iram_write(iram_write),
    .iram_address(iram_address),
    .iram_wdata(iram_wdata),
    .iram_byte_en(iram_byte_en),
    .iram_ack(iram_ack),
    .iram_rdata(iram_rdata),
    .instr_address(instr_address),
    .instr_data(instr_data)
  );

sdram_controller  sdram_controller_inst (
    .clock(clock),
    .reset(reset),
    .DRAM_ADDR(DRAM_ADDR),
    .DRAM_BA(DRAM_BA),
    .DRAM_CKE(DRAM_CKE),
    .DRAM_DQ(DRAM_DQ),
    .DRAM_CS_N(DRAM_CS_N),
    .DRAM_LDQM(DRAM_LDQM),
    .DRAM_RAS_N(DRAM_RAS_N),
    .DRAM_UDQM(DRAM_UDQM),
    .DRAM_WE_N(DRAM_WE_N),
    .DRAM_CAS_N(DRAM_CAS_N),
    .sdram_request(sdram_request),
    .sdram_write(sdram_write),
    .sdram_address(sdram_address),
    .sdram_wdata(sdram_wdata),
    .sdram_byte_en(sdram_byte_en),
    .sdram_rdata(sdram_rdata),
    .sdram_ack(sdram_ack)
  );

hwregs  hwregs_inst (
    .clock(clock),
    .reset(reset),
    .hwregs_request(hwregs_request),
    .hwregs_write(hwregs_write),
    .hwregs_address(hwregs_address),
    .hwregs_wdata(hwregs_wdata),
    .hwregs_byte_en(hwregs_byte_en),
    .hwregs_ack(hwregs_ack),
    .hwregs_rdata(hwregs_rdata),
    .seven_segment(seven_segment),
    .seven_segment_brightness(seven_segment_brightness)
  );

seven_segment  seven_segment_inst (
    .clock(clock),
    .seven_segment(seven_segment),
    .seven_segment_brightness(seven_segment_brightness),
    .HEX0(HEX0),
    .HEX1(HEX1),
    .HEX2(HEX2),
    .HEX3(HEX3),
    .HEX4(HEX4),
    .HEX5(HEX5)
  );

wire _unused_ok = &{1'b0, cpu_address, instr_address};
endmodule