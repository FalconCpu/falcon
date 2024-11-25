
//=======================================================
//  This code is generated by Terasic System Builder
//=======================================================

module Falcon(

	//////////// Audio //////////
	input 		          		AUD_ADCDAT,
	inout 		          		AUD_ADCLRCK,
	inout 		          		AUD_BCLK,
	output		          		AUD_DACDAT,
	inout 		          		AUD_DACLRCK,
	output		          		AUD_XCK,

	//////////// CLOCK //////////
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



//=======================================================
//  REG/WIRE declarations
//=======================================================
wire               clock;
wire               reset;
wire               locked;

// Signals from the CPU
wire               cpu_request;
wire  [31:0]       cpu_address;
wire               cpu_write;
wire  [3:0]        cpu_wstrb;
wire  [31:0]       cpu_wdata;
wire  [31:0]       cpu_rdata;
wire               cpu_ack;
wire  [31:0]       instr_addr;

// Signals from the address decoder
wire               dmem_request;
wire               hwregs_request;
wire               imem_request;
wire               error_request;

// signals from the memory
wire  [31:0]       dmem_rdata;
wire               dmem_ack;
wire               dcache_request;
wire               dcache_write;
wire  [25:0]       dcache_address;
wire               dcache_burst;
wire  [31:0]       dcache_wdata;
wire  [3:0]        dcache_byte_en;

// signals from the instruction memory
wire  [31:0]       imem_rdata;
wire               imem_ack;
wire [31:0]        instr_data;

// signals from the hardware registers
wire  [31:0]       hwregs_rdata;
wire               hwregs_ack;
wire  [23:0]       seven_seg_data;
wire               uart_tx_valid;
wire [7:0]         uart_tx_data;

// signals from the UART
wire  [7:0]        uart_rx_data;
wire               uart_tx_complete;
wire               uart_rx_complete;
wire               UART_TX;
wire               UART_RX;

// Signals from the sdram arbiter
wire [31:0]        dcache_rdata;
wire               dcache_valid;
wire               dcache_complete;
wire [31:0]        vga_rdata;
wire               vga_valid;
wire               vga_complete;
wire               sdram_request;
wire               sdram_write;
wire [3:0]         sdram_master;
wire [25:0]        sdram_address;
wire [31:0]        sdram_wdata;
wire [3:0]         sdram_byte_en;
wire               sdram_burst;

// Signals from the VGA
wire               vga_request;
wire  [25:0]       vga_address;

// Signals from the sdram_controller
wire [31:0]        sdram_rdata;
wire [3:0]         sdram_valid;
wire [3:0]         sdram_complete;

// Signals from the mouse
wire [9:0] mouse_x;
wire [9:0] mouse_y;
wire [2:0] mouse_buttons;

wire [31:0]        debug_pc;

//=======================================================
//  Structural coding
//=======================================================

assign reset = ~locked || ~KEY[0];

assign cpu_rdata = dmem_rdata | hwregs_rdata | imem_rdata;
assign cpu_ack = dmem_ack | hwregs_ack | imem_ack;

assign UART_RX = GPIO_0[35];
assign GPIO_0[34] = UART_TX;

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
	.instr_addr(instr_addr),
	.instr_data(instr_data),
    .cpu_request(cpu_request),
    .cpu_address(cpu_address),
    .cpu_write(cpu_write),
    .cpu_wstrb(cpu_wstrb),
    .cpu_wdata(cpu_wdata),
    .cpu_rdata(cpu_rdata),
    .cpu_ack(cpu_ack),
	.debug_pc(debug_pc)
  );

address_decoder  address_decoder_inst (
    .cpu_request(cpu_request),
    .cpu_address(cpu_address),
    .dmem_request(dmem_request),
	.imem_request(imem_request),
    .hwregs_request(hwregs_request),
    .error_request(error_request)
  );

instruction_memory  instruction_memory_inst (
    .clock(clock),
    .request(imem_request),
    .address(cpu_address[15:0]),
    .write(cpu_write),
    .wstrb(cpu_wstrb),
    .wdata(cpu_wdata),
    .rdata(imem_rdata),
    .ack(imem_ack),
	.instr_address(instr_addr[15:2]),
    .instr_data(instr_data)
  );

  data_cache  data_cache_inst (
    .clock(clock),
    .reset(reset),
    .cpu_request(dmem_request),
    .cpu_address(cpu_address[25:0]),
    .cpu_write(cpu_write),
    .cpu_wstrb(cpu_wstrb),
    .cpu_wdata(cpu_wdata),
    .cpu_rdata(dmem_rdata),
    .cpu_ack(dmem_ack),
    .sdram_request(dcache_request),
    .sdram_write(dcache_write),
    .sdram_address(dcache_address),
    .sdram_burst(dcache_burst),
    .sdram_wdata(dcache_wdata),
    .sdram_byte_en(dcache_byte_en),
    .sdram_rdata(dcache_rdata),
    .sdram_valid(dcache_valid),
    .sdram_complete(dcache_complete)
  );


hwregs  hwregs_inst (
    .clock(clock),
    .reset(reset),
    .request(hwregs_request),
    .address(cpu_address[15:0]),
    .write(cpu_write),
    .wstrb(cpu_wstrb),
    .wdata(cpu_wdata),
    .rdata(hwregs_rdata),
    .ack(hwregs_ack),
    .LEDR(LEDR),
    .seven_seg_data(seven_seg_data),
    .SW(SW),
    .KEY(KEY),
	  .uart_rx_complete(uart_rx_complete),
    .uart_rx_data(uart_rx_data),
    .uart_tx_valid(uart_tx_valid),
    .uart_tx_data(uart_tx_data),
    .uart_tx_complete(uart_tx_complete),
    .mouse_x(mouse_x),
    .mouse_y(mouse_y),
    .mouse_buttons(mouse_buttons)
);

reg prev_key;
reg  [23:0]        latched_pc;
always @(posedge clock) begin
	prev_key <= KEY[3];
	if (prev_key == 1'b1 && KEY[3] == 1'b0)
		latched_pc <= debug_pc[23:0];
end
wire [23:0] seven_seg_data_muxed = KEY[3] ? seven_seg_data : latched_pc;


seven_seg  seven_seg_inst (
    .seven_seg_data(seven_seg_data_muxed),
    .HEX0(HEX0),
    .HEX1(HEX1),
    .HEX2(HEX2),
    .HEX3(HEX3),
    .HEX4(HEX4),
    .HEX5(HEX5)
);

uart  uart_inst (
    .clock(clock),
    .reset(reset),
    .UART_RX(UART_RX),
    .UART_TX(UART_TX),
    .rx_complete(uart_rx_complete),
    .rx_data(uart_rx_data),
    .tx_valid(uart_tx_valid),
    .tx_data(uart_tx_data),
    .tx_complete(uart_tx_complete)
);

sdram_arbiter  sdram_arbiter_inst (
    .clock(clock),
    .reset(reset),
    .dcache_request(dcache_request),
    .dcache_write(dcache_write),
    .dcache_address(dcache_address),
    .dcache_burst(dcache_burst),
    .dcache_wdata(dcache_wdata),
    .dcache_byte_en(dcache_byte_en),
    .dcache_rdata(dcache_rdata),
    .dcache_valid(dcache_valid),
    .dcache_complete(dcache_complete),
    .vga_request(vga_request),
    .vga_address(vga_address),
    .vga_rdata(vga_rdata),
    .vga_valid(vga_valid),
    .vga_complete(vga_complete),
    .sdram_request(sdram_request),
    .sdram_write(sdram_write),
    .sdram_master(sdram_master),
    .sdram_address(sdram_address),
    .sdram_wdata(sdram_wdata),
    .sdram_byte_en(sdram_byte_en),
    .sdram_rdata(sdram_rdata),
    .sdram_valid(sdram_valid),
    .sdram_burst(sdram_burst),
    .sdram_complete(sdram_complete)
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
    .sdram_master(sdram_master),
    .sdram_write(sdram_write),
    .sdram_address(sdram_address),
    .sdram_wdata(sdram_wdata),
    .sdram_byte_en(sdram_byte_en),
    .sdram_burst(sdram_burst),
    .sdram_rdata(sdram_rdata),
    .sdram_valid(sdram_valid),
    .sdram_complete(sdram_complete)
  );  

  vga_output  vga_output_inst (
    .clock(clock),
    .reset(reset),
    .VGA_CLK(VGA_CLK),
    .VGA_R(VGA_R),
    .VGA_G(VGA_G),
    .VGA_B(VGA_B),
    .VGA_HS(VGA_HS),
    .VGA_VS(VGA_VS),
    .vga_request(vga_request),
    .vga_address(vga_address),
    .vga_rdata(vga_rdata),
    .vga_valid(vga_valid),
    .vga_complete(vga_complete),
    .mouse_x(mouse_x),
    .mouse_y(mouse_y)
  );

  assign VGA_BLANK_N = 1'b1;
  assign VGA_SYNC_N  = 1'b0;

mouse_interface  mouse_interface_inst (
    .clock(clock),
    .reset(reset),
    .PS2_CLK(PS2_CLK),
    .PS2_DAT(PS2_DAT),
    .mouse_x(mouse_x),
    .mouse_y(mouse_y),
    .mouse_buttons(mouse_buttons)
  );


endmodule
