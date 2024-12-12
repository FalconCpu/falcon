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
// E0000030  SIM_MODE       R    1 if in simulation mode, 0 if in hardware
// E0000080  BLIT_CMD       R/W  BLit command
// E0000084  BLIT_WIDTH     R/W  Blitter width
// E0000088  BLIT_HEIGHT    R/W  
// E000008C  BLIT_FGCOLOR   R/W  
// E0000090  BLIT_BGCOLOR   R/W  
// E0000094  BLIT_DEST_ADDR R/W  
// E0000098  BLIT_DEST_BPR  R/W  
// E000009C  BLIT_DEST_X    R/W  
// E00000A0  BLIT_DEST_Y    R/W  
// E00000A4  BLIT_SRC_ADDR  R/W  
// E00000A8  BLIT_SRC_BPR   R/W  
// E00000AC  BLIT_SRC_X     R/W  
// E00000B0  BLIT_SRC_Y     R/W  
// E00000B4  BLIT_CLIP_X1   R/W  
// E00000B8  BLIT_CLIP_Y1   R/W  
// E00000BC  BLIT_CLIP_X2   R/W  
// E00000C0  BLIT_CLIP_Y2   R/W  
// E00000C4  BLIT_TRANSPARENT_COLOR 

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
    output reg          ack,

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

    input [7:0]      keyboard_code,
    input            keyboard_strobe,

    // connections to the blitter
    output reg [7:0]  blit_cmd,        // Blitter command - see above for list of commands
    output reg [15:0] blit_width,      // Width of the blit
    output reg [15:0] blit_height,     // Height of the blit
    output reg [7:0]  blit_fgcolor,    // Foreground color
    output reg [7:0]  blit_bgcolor,    // Background color
    output reg [25:0] blit_dest_addr,   // start address of the destination buffer
    output reg [15:0] blit_dest_bpr,    // bytes per row
    output reg [15:0] blit_dest_x,      // x coordinate for destination of blit
    output reg [15:0] blit_dest_y,      // y coordinate for destination of blit
    output reg [25:0] blit_src_addr,    // start address of the source buffer
    output reg [15:0] blit_src_bpr,     // bytes per row
    output reg [15:0] blit_src_x,       // x coordinate for source of blit
    output reg [15:0] blit_src_y,       // 
    output reg [15:0] blit_clip_x1,
    output reg [15:0] blit_clip_y1,
    output reg [15:0] blit_clip_x2,
    output reg [15:0] blit_clip_y2,
    output reg [8:0]  blit_transparent_color,

    output reg       blit_start,
    input            blit_busy
);

reg uart_rx_read;                   // Set to indicate we have read a byte from the uart
reg keyboard_read; 
wire uart_rx_valid;                 // Set to indicate there is valid data in the uart rx fifo
wire [7:0] latched_uart_rx_data;    // data at the front of the uart rx fifo
wire [9:0] uart_tx_slots_free;      // Number of slots free in the uart tx fifo
wire [7:0] latched_keyboard_code;   // data at the front of the keyboard rx fifo
wire       keyboard_valid;

// synthesis translate_off
integer  uart_log_file;
initial 
    uart_log_file = $fopen("rtl_uart.log","w");
// synthesis translate_on



always @(posedge clock) begin
    ack <= request;
    uart_rx_read <= 1'b0;
    keyboard_read <= 1'b0;
    blit_start <= 1'b0;

    if (request && write)
        case (address)
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
                if (blit_busy)
                    $display("ERROR: Blit command while blit_busy");
                blit_cmd <= wdata[7:0];
                blit_start <= 1'b1;
            end

            16'h0084: blit_width     <= wdata[15:0];      
            16'h0088: blit_height    <= wdata[15:0];     
            16'h008C: blit_fgcolor   <= wdata[7:0];    
            16'h0090: blit_bgcolor   <= wdata[7:0];    
            16'h0094: blit_dest_addr <= wdata[25:0];  
            16'h0098: blit_dest_bpr  <= wdata[15:0];   
            16'h009C: blit_dest_x    <= wdata[15:0];     
            16'h00A0: blit_dest_y    <= wdata[15:0];     
            16'h00A4: blit_src_addr  <= wdata[25:0];   
            16'h00A8: blit_src_bpr   <= wdata[15:0];    
            16'h00AC: blit_src_x     <= wdata[15:0];      
            16'h00B0: blit_src_y     <= wdata[15:0];      
            16'h00B4: blit_clip_x1   <= wdata[15:0];
            16'h00B8: blit_clip_y1   <= wdata[15:0];
            16'h00BC: blit_clip_x2   <= wdata[15:0];
            16'h00C0: blit_clip_y2   <= wdata[15:0];
            16'h00C4: blit_transparent_color     <= wdata[8:0];
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
            
            16'h0030:  begin 
                // Simulation mode flag. Reads as 1 in simulation, 0 in hardware
                rdata <= 32'h0;
                // synthesis translate_off
                rdata <= 32'h1;
                // synthesis translate_on
            end
            16'h0080: rdata <= blit_busy ? blit_cmd : 32'h00;
            16'h0084: rdata <= blit_width;
            16'h0088: rdata <= blit_height;
            16'h008C: rdata <= blit_fgcolor;
            16'h0090: rdata <= blit_bgcolor;
            16'h0094: rdata <= blit_dest_addr;
            16'h0098: rdata <= blit_dest_bpr;
            16'h009C: rdata <= blit_dest_x;
            16'h00A0: rdata <= blit_dest_y;
            16'h00A4: rdata <= blit_src_addr;
            16'h00A8: rdata <= blit_src_bpr;
            16'h00AC: rdata <= blit_src_x;
            16'h00B0: rdata <= blit_src_y;
            16'h00B4: rdata <= blit_clip_x1;
            16'h00B8: rdata <= blit_clip_y1;
            16'h00BC: rdata <= blit_clip_x2;
            16'h00C0: rdata <= blit_clip_y2;
            16'h00C4: rdata <= blit_transparent_color;

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