`timescale 1ns / 1ns

// Module for the hardware registers
//
// This creates a 64kb block sitting at address 0xE0000000 in the CPU address space.
//
// ADDRESS   REGISTER       R/W  DESCRIPTION
// E0000000  SEVEN_SEG      R/W  6 digit hexadecimal seven segment display
// E0000004  LEDR           R/W  10 LEDs
// E0000008  SW             R    10 Switches    
// E000000C  KEY            R    4 Push buttons
// E0000010  UART_TX        R/W  Write = byte of data to transmit, read = number of slots free in fifo
// E0000014  UART_RX        R    1 byte of data from the uart, -1 if no data
// E0000018  MOUSE_X        R    10 bit mouse x position
// E000001C  MOUSE_Y        R    10 bit mouse y position
// E0000020  MOUSE_BTN      R    3 bit mouse buttons
// E0000024  KEYBOARD       R    Scan codes from the keyboard, -1 if no data
// E0000028  VGA_YPOS       R    10 bit y position of the VGA display
// E0000030  SIM_MODE       R    1 if in simulation mode, 0 if in hardware
// E0000034
// E0000038
// E000003C
// E0000040  DEBUG0         R
// E0000044  DEBUG1         R
// E0000048  DEBUG2         R
// E000004C  DEBUG3         R
// E0000050  DEBUG4         R
// E0000054  DEBUG5         R
// E0000058  DEBUG6         R
// E000005C  DEBUG7         R
// E0000080  BLIT_CMD       R/W  BLit command
// E0000084  BLIT_ARG1      R/W  Blitter Arguments
// E0000088  BLIT_ARG2      R/W  
// E000008C  BLIT_ARG3      R/W  

module  hwregs(
    // Connection to the cpu bus
    input               clock,
    input               reset,
    input               request,
    input [15:0]        address,
    input               write,
    input [3:0]         wstrb,
    input [31:0]        wdata,
    output reg [31:0]   rdata,
    output reg          valid,

    // connections to the chip pins
    output reg       [9:0]		LEDR,
    output reg       [23:0]     seven_seg_data,
	input 		     [9:0]		SW,
    input 		     [3:0]		KEY,

    input            uart_rx_complete, // Pulsed to indicate a byte has been received from the channel
    input [7:0]      uart_rx_data,     // The data received

    output           uart_tx_valid,    // Asserted to indicate data is ready to be transmitted
    output [7:0]     uart_tx_data,     // The data to transmit
    input            uart_tx_complete, // Pulsed to indicate transmisssion is complete

    input [9:0]      mouse_x,
    input [9:0]      mouse_y,
    input [2:0]      mouse_buttons,
    input [9:0]      vga_ypos,

    input [7:0]      keyboard_code,
    input            keyboard_strobe,

    // connections to the blitter
    output [103:0]     blit_cmd,        // Blitter command - see above for list of commands
    output reg         blit_start,
    input [7:0]        blit_slots_free,
    input [255:0]      debug
);

reg uart_rx_read;                   // Set to indicate we have read a byte from the uart
reg keyboard_read; 
wire uart_rx_valid;                 // Set to indicate there is valid data in the uart rx fifo
wire [7:0] latched_uart_rx_data;    // data at the front of the uart rx fifo
wire [9:0] uart_tx_slots_free;      // Number of slots free in the uart tx fifo
wire [7:0] latched_keyboard_code;   // data at the front of the keyboard rx fifo
wire       keyboard_valid;
reg [31:0] blit_arg1, blit_arg2, blit_arg3;
reg [7:0]  blit_cmd_cmd;

// synthesis translate_off
integer  uart_log_file;
initial 
    uart_log_file = $fopen("rtl_uart.log","w");
// synthesis translate_on

assign blit_cmd = {blit_cmd_cmd, blit_arg3, blit_arg2, blit_arg1};

always @(posedge clock) begin
    valid <= request;
    uart_rx_read <= 1'b0;
    keyboard_read <= 1'b0;
    blit_start <= 1'b0;

    if (request && write)
        case (address & 16'hfffc)
            16'h0000: begin seven_seg_data <= wdata[23:0]; $display("seven_seg_data = %h", wdata[23:0]); end
            16'h0004: begin LEDR <= wdata[9:0]; $display("LEDR = %b", wdata[9:0]); end
            16'h0010: begin 
                // transmit data to uart. Actual operation is handled by fifo module below.
                // But here we simply log the transaction to the log file.
                // synthesis translate_off
                $fwrite(uart_log_file,"%c",wdata[7:0]);
                // synthesis translate_on
            end 

            16'h0080: begin 
                blit_cmd_cmd <= wdata[7:0];
                blit_start <= 1'b1;
            end
            16'h0084: begin
                if (wstrb[0]) blit_arg1[7:0]   <= wdata[7:0];
                if (wstrb[1]) blit_arg1[15:8]  <= wdata[15:8];
                if (wstrb[2]) blit_arg1[23:16] <= wdata[23:16];
                if (wstrb[3]) blit_arg1[31:24] <= wdata[31:24];
            end

            16'h0088: begin
                if (wstrb[0]) blit_arg2[7:0]   <= wdata[7:0];
                if (wstrb[1]) blit_arg2[15:8]  <= wdata[15:8];
                if (wstrb[2]) blit_arg2[23:16] <= wdata[23:16];
                if (wstrb[3]) blit_arg2[31:24] <= wdata[31:24];
            end
                
            16'h008C: begin
                if (wstrb[0]) blit_arg3[7:0]   <= wdata[7:0];
                if (wstrb[1]) blit_arg3[15:8]  <= wdata[15:8];
                if (wstrb[2]) blit_arg3[23:16] <= wdata[23:16];
                if (wstrb[3]) blit_arg3[31:24] <= wdata[31:24];
            end
        endcase

    if (request && !write)
        case (address)
            16'h0000: rdata <= {8'h0, seven_seg_data};
            16'h0004: rdata <= {22'h0, LEDR};
            16'h0008: rdata <= {22'h0, SW};
            16'h000C: rdata <= {28'h0, ~KEY};
            16'h0010: rdata <= {22'h0, uart_tx_slots_free};
            16'h0014: begin 
                if (uart_rx_valid) begin
                    rdata <= {24'h0, latched_uart_rx_data};
                    uart_rx_read <= 1'b1;
                end else
                    rdata <= -1;
                end
            16'h0018: rdata <= {22'h0, mouse_x};
            16'h001C: rdata <= {22'h0, mouse_y};
            16'h0020: rdata <= {29'h0, mouse_buttons};
            16'h0024: begin
                if (keyboard_valid) begin
                    rdata <= {24'h0, latched_keyboard_code};
                    keyboard_read <= 1'b1;
                end else
                    rdata <= -1;
                end 
                
            16'h0028: rdata <= {22'h0, vga_ypos};
            
            16'h0030:  begin 
                // Simulation mode flag. Reads as 1 in simulation, 0 in hardware
                rdata <= 32'h0;
                // synthesis translate_off
                rdata <= 32'h1;
                // synthesis translate_on
            end

            16'h0040: rdata <= debug[31:0];
            16'h0044: rdata <= debug[63:32];
            16'h0048: rdata <= debug[95:64];
            16'h004C: rdata <= debug[127:96];
            16'h0050: rdata <= debug[159:128];
            16'h0054: rdata <= debug[191:160];
            16'h0058: rdata <= debug[223:192];
            16'h005c: rdata <= debug[255:224];

            16'h0080: rdata <= blit_slots_free;
            16'h0084: rdata <= blit_arg1;
            16'h0088: rdata <= blit_arg2;
            16'h008C: rdata <= blit_arg3;

            default:  rdata <= 32'h0;
        endcase
    else    
        rdata <= 32'h0;
end

wire uart_tx_write_select = (address== 16'h0010) && write && request;
byte_fifo  uart_tx_fifo (
    .clk(clock),
    .reset(reset),
    .write_enable(uart_tx_write_select),
    .write_data(wdata[7:0]),
    .read_enable(uart_tx_complete),
    .read_data(uart_tx_data),
    .slots_free(uart_tx_slots_free),
    .not_empty(uart_tx_valid)
  );

byte_fifo  uart_rx_fifo (
    .clk(clock),
    .reset(reset),
    .write_enable(uart_rx_complete),
    .write_data(uart_rx_data),
    .read_enable(uart_rx_read),
    .read_data(latched_uart_rx_data),
    .slots_free(),
    .not_empty(uart_rx_valid)
  );

byte_fifo  keyboard_fifo (
    .clk(clock),
    .reset(reset),
    .write_enable(keyboard_strobe),
    .write_data(keyboard_code),
    .read_enable(keyboard_read),
    .read_data(latched_keyboard_code),
    .slots_free(),
    .not_empty(keyboard_valid)
  );


endmodule