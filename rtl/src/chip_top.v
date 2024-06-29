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
wire           cpu_burst = 1'b0;
wire           cpu_valid;
wire           cpu_complete;

wire           gpu_request;
wire           gpu_write;
wire [25:0]    gpu_address;
wire [31:0]    gpu_wdata;
wire [3:0]     gpu_byte_en;
wire [31:0]    gpu_rdata;
wire           gpu_burst;
wire           gpu_valid;
wire           gpu_complete;

wire           iram_request;
wire           iram_write;
wire [15:0]    iram_address;
wire [31:0]    iram_wdata;
wire [3:0]     iram_byte_en;
wire [31:0]    iram_rdata;
wire           iram_valid;

wire           cpu_to_sdram_request;
wire           cpu_to_sdram_write;
wire [25:0]    cpu_to_sdram_address;
wire [31:0]    cpu_to_sdram_wdata;
wire [3:0]     cpu_to_sdram_byte_en;
wire [31:0]    cpu_to_sdram_rdata;
wire           cpu_to_sdram_valid;
wire           cpu_to_sdram_burst;
wire           cpu_to_sdram_complete;


wire           sdram_request;
wire           sdram_write;
wire [25:0]    sdram_address;
wire [31:0]    sdram_wdata;
wire [3:0]     sdram_byte_en;
wire [31:0]    sdram_rdata;
wire [3:0]     sdram_valid;
wire           sdram_burst;
wire [3:0]     sdram_complete;
wire [3:0]     sdram_master;

wire           hwregs_request;
wire           hwregs_write;
wire [15:0]    hwregs_address;
wire [31:0]    hwregs_wdata;
wire [3:0]     hwregs_byte_en;
wire [31:0]    hwregs_rdata;
wire           hwregs_valid;

wire           pram_request;
wire           pram_write;
wire [15:0]    pram_address;
wire [31:0]    pram_wdata;
wire [3:0]     pram_byte_en;
wire [31:0]    pram_rdata;
wire           pram_valid;

wire           vga_request;
wire [25:0]    vga_address;
wire [31:0]    vga_rdata;
wire           vga_valid;
wire           vga_complete;


// signals driven by hwregs
wire [23:0]    seven_segment;
wire [7:0]     seven_segment_brightness;
wire           uart_tx_ready;      // Asserted to indicate data is ready to be transmitted
wire [7:0]     uart_tx_word;       // The data to transmit

wire [15:0] gpu_x;               // X location on screen
wire [15:0] gpu_y;               // Y location on screen
wire [15:0] gpu_width;           // Width of operation (in pixels)
wire [15:0] gpu_height;          // Height of operation (in pixels)
wire [15:0] gpu_pattern_offset;  // offset within pattern for pattern
wire [7:0]  gpu_pattern_width;   // width of pattern in bytes
wire [1:0]  gpu_pattern_depth;   // 1,2,4,8 bits per pixel
wire [8:0]  gpu_color0;          // Background color (256=transparant)
wire [8:0]  gpu_color1;          // Foreground color
wire [7:0]  gpu_command;         // Action to perform
wire        gpu_start;           // Start pulse
wire [15:0] gpu_clip_x1;         // Clipping region
wire [15:0] gpu_clip_y1;
wire [15:0] gpu_clip_x2;
wire [15:0] gpu_clip_y2;
wire        gpu_busy;


// signals driven by uart
wire       uart_rx_complete;  // Pulsed to indicate a byte has been received from the channel
wire [7:0] uart_rx_word;   
wire       uart_tx_complete;   // Pulsd to indicate the byte has been transmitted
wire [31:0] hwregs_screen_address;
wire [8:0]  hwregs_screen_blank;

// signals driven by mouse controller 
wire [9:0] mouse_x, mouse_y;
wire [2:0] mouse_buttons;

// signals driven by the keyboard
wire [7:0] keyboard_data;
wire       keyboard_strobe;



// signals driven by VGA

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
    .cpu_valid(cpu_valid),
    .instr_address(instr_address),
    .instr_data(instr_data)
  );

  address_decoder  address_decoder_inst (
    .clock(clock),
    .reset(reset),
    .cpu_request(cpu_request),
    .cpu_write(cpu_write),
    .cpu_address(cpu_address),
    .cpu_wdata(cpu_wdata),
    .cpu_byte_en(cpu_byte_en),
    .cpu_rdata(cpu_rdata),
    .cpu_valid(cpu_valid),
    .cpu_complete(cpu_complete),
    .iram_request(iram_request),
    .iram_write(iram_write),
    .iram_address(iram_address),
    .iram_wdata(iram_wdata),
    .iram_byte_en(iram_byte_en),
    .iram_rdata(iram_rdata),
    .iram_valid(iram_valid),
    .sdram_request(cpu_to_sdram_request),
    .sdram_write(cpu_to_sdram_write),
    .sdram_address(cpu_to_sdram_address),
    .sdram_wdata(cpu_to_sdram_wdata),
    .sdram_byte_en(cpu_to_sdram_byte_en),
    .sdram_rdata(cpu_to_sdram_rdata),
    .sdram_valid(cpu_to_sdram_valid),
    .sdram_burst(cpu_to_sdram_burst),
    .sdram_complete(cpu_to_sdram_complete),
    .hwregs_request(hwregs_request),
    .hwregs_write(hwregs_write),
    .hwregs_address(hwregs_address),
    .hwregs_wdata(hwregs_wdata),
    .hwregs_byte_en(hwregs_byte_en),
    .hwregs_rdata(hwregs_rdata),
    .hwregs_valid(hwregs_valid),
    .pram_request(pram_request),
    .pram_write(pram_write),
    .pram_address(pram_address),
    .pram_wdata(pram_wdata),
    .pram_byte_en(pram_byte_en),
    .pram_valid(pram_valid),
    .pram_rdata(pram_rdata)
  );

  sdram_arbiter  sdram_arbiter_inst (
    .clock(clock),
    .reset(reset),
    .cpu_request(cpu_to_sdram_request),
    .cpu_write(cpu_to_sdram_write),
    .cpu_address(cpu_to_sdram_address),
    .cpu_burst(cpu_to_sdram_burst),
    .cpu_wdata(cpu_to_sdram_wdata),
    .cpu_byte_en(cpu_to_sdram_byte_en),
    .cpu_rdata(cpu_to_sdram_rdata),
    .cpu_valid(cpu_to_sdram_valid),
    .cpu_complete(cpu_to_sdram_complete),
    .vga_request(vga_request),
    .vga_address(vga_address),
    .vga_rdata(vga_rdata),
    .vga_valid(vga_valid),
    .vga_complete(vga_complete),
    .gpu_request(gpu_request),
    .gpu_write(gpu_write),
    .gpu_address(gpu_address),
    .gpu_wdata(gpu_wdata),
    .gpu_burst(gpu_burst),
    .gpu_byte_en(gpu_byte_en),
    .gpu_rdata(gpu_rdata),
    .gpu_valid(gpu_valid),
    .gpu_complete(gpu_complete),
    .sdram_request(sdram_request),
    .sdram_master(sdram_master),
    .sdram_write(sdram_write),
    .sdram_address(sdram_address),
    .sdram_wdata(sdram_wdata),
    .sdram_byte_en(sdram_byte_en),
    .sdram_rdata(sdram_rdata),
    .sdram_valid(sdram_valid),
    .sdram_burst(sdram_burst),
    .sdram_complete(sdram_complete)
  );

instruction_ram  instruction_ram_inst (
    .clock(clock),
    .iram_request(iram_request),
    .iram_write(iram_write),
    .iram_address(iram_address),
    .iram_wdata(iram_wdata),
    .iram_byte_en(iram_byte_en),
    .iram_valid(iram_valid),
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
    .sdram_master(sdram_master),
    .sdram_request(sdram_request),
    .sdram_write(sdram_write),
    .sdram_address(sdram_address),
    .sdram_wdata(sdram_wdata),
    .sdram_byte_en(sdram_byte_en),
    .sdram_rdata(sdram_rdata),
    .sdram_valid(sdram_valid),
    .sdram_burst(sdram_burst),
    .sdram_complete(sdram_complete)
  );

  hwregs  hwregs_inst (
    .clock(clock),
    .reset(reset),
    .hwregs_request(hwregs_request),
    .hwregs_write(hwregs_write),
    .hwregs_address(hwregs_address),
    .hwregs_wdata(hwregs_wdata),
    .hwregs_byte_en(hwregs_byte_en),
    .hwregs_valid(hwregs_valid),
    .hwregs_rdata(hwregs_rdata),
    .hwreg_seven_segment(seven_segment),
    .hwreg_seven_segment_brightness(seven_segment_brightness),
    .hwregs_screen_blank(hwregs_screen_blank),
    .hwregs_screen_addr(hwregs_screen_address),
    .uart_rx_complete(uart_rx_complete),
    .uart_rx_word(uart_rx_word),
    .uart_tx_ready(uart_tx_ready),
    .uart_tx_word(uart_tx_word),
    .uart_tx_complete(uart_tx_complete),
    .mouse_x(mouse_x),
    .mouse_y(mouse_y),
    .mouse_buttons(mouse_buttons),
    .keyboard_data(keyboard_data),
    .keyboard_strobe(keyboard_strobe),
    .gpu_x(gpu_x),
    .gpu_y(gpu_y),
    .gpu_width(gpu_width),
    .gpu_height(gpu_height),
    .gpu_pattern_offset(gpu_pattern_offset),
    .gpu_pattern_width(gpu_pattern_width),
    .gpu_pattern_depth(gpu_pattern_depth),
    .gpu_color0(gpu_color0),
    .gpu_color1(gpu_color1),
    .gpu_command(gpu_command),
    .gpu_start(gpu_start),
    .gpu_busy(gpu_busy),
    .gpu_clip_x1(gpu_clip_x1),
    .gpu_clip_y1(gpu_clip_y1),
    .gpu_clip_x2(gpu_clip_x2),
    .gpu_clip_y2(gpu_clip_y2),
    .LEDR(LEDR[8:0])
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

uart  uart_inst (
    .clock(clock),
    .reset(reset),
    .UART_RX(GPIO_0[35]),
    .UART_TX(GPIO_0[34]),
    .rx_complete(uart_rx_complete),
    .rx_word(uart_rx_word),
    .tx_ready(uart_tx_ready),
    .tx_word(uart_tx_word),
    .tx_complete(uart_tx_complete),
    .uart_led(LEDR[9])
  );
  
vga_output  vga_output_inst (
    .clock(clock),
    .reset(reset),
    .screen_blank(hwregs_screen_blank),
    .screen_address(hwregs_screen_address),
    .VGA_BLANK_N(VGA_BLANK_N),
    .VGA_B(VGA_B),
    .VGA_CLK(VGA_CLK),
    .VGA_G(VGA_G),
    .VGA_HS(VGA_HS),
    .VGA_R(VGA_R),
    .VGA_SYNC_N(VGA_SYNC_N),
    .VGA_VS(VGA_VS),
    .vga_request(vga_request),
    .vga_address(vga_address),
    .vga_complete(vga_complete),
    .vga_rdata(vga_rdata),
    .vga_valid(vga_valid),
    .mouse_x(mouse_x),
    .mouse_y(mouse_y)
  );

mouse_interface  mouse_interface_inst (
    .clock(clock),
    .reset(reset),
    .PS2_CLK(PS2_CLK),
    .PS2_DAT(PS2_DAT),
    .mouse_x(mouse_x),
    .mouse_y(mouse_y),
    .mouse_buttons(mouse_buttons)
  );

keyboard_if  keyboard_if_inst (
    .clock(clock),
    .reset(reset),
    .PS2_CLK2(PS2_CLK2),
    .PS2_DAT2(PS2_DAT2),
    .keyboard_code(keyboard_data),
    .keyboard_strobe(keyboard_strobe)
  );

  gpu  gpu_inst (
    .clock(clock),
    .reset(reset),
    .pram_request(pram_request),
    .pram_write(pram_write),
    .pram_address(pram_address),
    .pram_wdata(pram_wdata),
    .pram_byte_en(pram_byte_en),
    .pram_valid(pram_valid),
    .pram_rdata(pram_rdata),
    .gpu_request(gpu_request),
    .gpu_write(gpu_write),
    .gpu_address(gpu_address),
    .gpu_wdata(gpu_wdata),
    .gpu_burst(gpu_burst),
    .gpu_byte_en(gpu_byte_en),
    .gpu_rdata(gpu_rdata),
    .gpu_valid(gpu_valid),
    .gpu_complete(gpu_complete),
    .gpu_x(gpu_x),
    .gpu_y(gpu_y),
    .gpu_width(gpu_width),
    .gpu_height(gpu_height),
    .gpu_pattern_offset(gpu_pattern_offset),
    .gpu_pattern_width(gpu_pattern_width),
    .gpu_pattern_depth(gpu_pattern_depth),
    .gpu_color0(gpu_color0),
    .gpu_color1(gpu_color1),
    .gpu_command(gpu_command),
    .gpu_start(gpu_start),
    .gpu_clip_x1(gpu_clip_x1),
    .gpu_clip_y1(gpu_clip_y1),
    .gpu_clip_x2(gpu_clip_x2),
    .gpu_clip_y2(gpu_clip_y2),
    .gpu_busy(gpu_busy),
    .gpu_screen_address(hwregs_screen_address)
  );


wire _unused_ok = &{1'b0, cpu_address, instr_address};
endmodule