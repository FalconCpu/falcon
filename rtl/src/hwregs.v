`timescale 1ns/1ns

module hwregs(
    input          clock,
    input          reset,

    // connection to the data bus
    input            hwregs_request,
    input            hwregs_write,
    input   [15:0]   hwregs_address,
    input   [31:0]   hwregs_wdata,
    input   [3:0]    hwregs_byte_en,
    output reg       hwregs_valid,
    output reg[31:0] hwregs_rdata,

    // Connections to peripherals
    input     [9:0]   mouse_x,
    input     [9:0]   mouse_y,
    input     [2:0]   mouse_buttons,
    output reg[23:0]  hwreg_seven_segment,
    output reg[7:0]   hwreg_seven_segment_brightness,

    input            uart_rx_complete,
    input [7:0]      uart_rx_word,
    output           uart_tx_ready,
    output [7:0]     uart_tx_word,
    input            uart_tx_complete,

    input [7:0]      keyboard_data,
    input            keyboard_strobe,
    

    output reg [15:0] gpu_x,               // X location on screen
    output reg [15:0] gpu_y,               // Y location on screen
    output reg [15:0] gpu_width,           // Width of operation (in pixels)
    output reg [15:0] gpu_height,          // Height of operation (in pixels)
    output reg [15:0] gpu_pattern_offset,  // offset within pattern for pattern
    output reg [7:0]  gpu_pattern_width,   // width of pattern in bytes
    output reg [1:0]  gpu_pattern_depth,   // 1,2,4,8 bits per pixel
    output reg [8:0]  gpu_color0,          // Background color (256=transparant)
    output reg [8:0]  gpu_color1,          // Foreground color
    output reg [7:0]  gpu_command,         // Action to perform
    output reg        gpu_start,           // Start pulse
    input             gpu_busy,
    output reg [15:0] gpu_clip_x1,         // Clipping region
    output reg [15:0] gpu_clip_y1,
    output reg [15:0] gpu_clip_x2,
    output reg [15:0] gpu_clip_y2,
    output reg [8:0]  hwregs_screen_blank,
    output reg [31:0] hwregs_screen_addr,
    output reg [8:0] LEDR
);

wire [23:0] pointers;

wire [7:0] uart_rx_fifo_data;
wire [9:0] uart_tx_slots_free;
wire       uart_rx_fifo_not_empty;

wire [7:0] keyboard_fifo_data;
wire       keyboard_fifo_not_empty;

reg [15:0] reg_x;
reg [15:0] reg_y;
reg [15:0] reg_width;
reg [15:0] reg_height;
reg [15:0] reg_pattern_offset;
reg [7:0]  reg_pattern_width;
reg [1:0]  reg_pattern_depth;
reg [8:0]  reg_color0;
reg [8:0]  reg_color1;

// synthesis translate_off
integer log_file;
initial begin
    log_file = $fopen("rtl_uart.log","w");
end
// synthesis translate_on


always @(posedge clock) begin
    hwregs_valid <= hwregs_request;
    hwregs_rdata <= 0;
    gpu_start    <= 1'b0;

    // Handle reset
    if (reset) begin
        hwreg_seven_segment <= 0;
        hwreg_seven_segment_brightness <= 8'hff;
        hwregs_screen_addr <= 26'h3f80000;
        hwregs_screen_blank <= 9'h0;
        gpu_clip_x1 <= 0;
        gpu_clip_y1 <= 0;
        gpu_clip_x2 <= 640;
        gpu_clip_y2 <= 480;
    end

    // ================================================
    //                 Write regs
    // ================================================
    if (hwregs_request && hwregs_write)
        case(hwregs_address)
            16'h0000: begin
                hwreg_seven_segment <= hwregs_wdata[23:0];
                $display("7SEG = %x",hwregs_wdata[23:0]);
            end

            16'h0004: LEDR                <= hwregs_wdata[8:0];
            16'h0008: begin end // mouse_x
            16'h000c: begin end // mouse_x
            16'h0010: begin end // mouse_buttons
            16'h0014: begin 
                // The fifo below will pick up the byte to transmit and load it into the fifo
                // synthesys translate_off
                $fwrite(log_file,"%c",hwregs_wdata[7:0]);
                // synthesys translate_on
            end 
            16'h0018: begin end // uart_rx
            16'h001C: hwregs_screen_blank <= hwregs_wdata[8:0];
            16'h0020: hwregs_screen_addr  <= hwregs_wdata[25:0];
            16'h0024: hwreg_seven_segment_brightness <= hwregs_wdata[7:0];
            16'h0028: begin end // keyboard

            16'h0040: reg_x      <= hwregs_wdata[15:0];
            16'h0044: reg_y      <= hwregs_wdata[15:0];
            16'h0048: reg_width  <= hwregs_wdata[15:0];
            16'h004C: reg_height <= hwregs_wdata[15:0];
            16'h0050: reg_pattern_offset <= hwregs_wdata[15:0];
            16'h0054: reg_pattern_width <= hwregs_wdata[7:0];
            16'h0058: reg_pattern_depth <= hwregs_wdata[1:0];
            16'h005C: reg_color0 <= hwregs_wdata[8:0];
            16'h0060: reg_color1 <= hwregs_wdata[8:0];            
            16'h0064: begin
                if (gpu_busy) 
                    $display("Time %t ERROR write to GPU while busy",$abstime);
                gpu_command <= hwregs_wdata[7:0];
                gpu_start          <= 1'b1;
                gpu_x              <= reg_x;
                gpu_y              <= reg_y;
                gpu_width          <= reg_width;
                gpu_height         <= reg_height;
                gpu_pattern_offset <= reg_pattern_offset;
                gpu_pattern_width  <= reg_pattern_width;
                gpu_pattern_depth  <= reg_pattern_depth;
                gpu_color0         <= reg_color0;
                gpu_color1         <= reg_color1;
            end
            16'h0080: gpu_clip_x1 <= hwregs_wdata[15:0];
            16'h0084: gpu_clip_y1 <= hwregs_wdata[15:0];
            16'h0088: gpu_clip_x2 <= hwregs_wdata[15:0];
            16'h008C: gpu_clip_y2 <= hwregs_wdata[15:0];
        endcase

    // ================================================
    //                     reads 
    // ================================================
    if (hwregs_request && !hwregs_write)
    case(hwregs_address)
        16'h0000: hwregs_rdata <= hwreg_seven_segment;
        16'h0004: hwregs_rdata <= LEDR;
        16'h0008: hwregs_rdata <= mouse_x;
        16'h000C: hwregs_rdata <= mouse_y;
        16'h0010: hwregs_rdata <= mouse_buttons;
        16'h0014: hwregs_rdata <= {22'b0, uart_tx_slots_free};
        16'h0018: hwregs_rdata <= uart_rx_fifo_not_empty ?  uart_rx_fifo_data : 32'hffffffff;
        16'h001c: hwregs_rdata <= hwregs_screen_blank;
        16'h0020: hwregs_rdata <= hwregs_screen_addr;
        16'h0024: hwregs_rdata <= hwreg_seven_segment_brightness;
        16'h0028: hwregs_rdata <= keyboard_fifo_not_empty ?  keyboard_fifo_data : 32'hffffffff;
        16'h0040: hwregs_rdata <= reg_x;
        16'h0044: hwregs_rdata <= reg_y;
        16'h0048: hwregs_rdata <= reg_width;
        16'h004C: hwregs_rdata <= reg_height;
        16'h0050: hwregs_rdata <= reg_pattern_offset;
        16'h0054: hwregs_rdata <= reg_pattern_width;
        16'h0058: hwregs_rdata <= reg_pattern_depth;
        16'h005C: hwregs_rdata <= reg_color0;
        16'h0060: hwregs_rdata <= reg_color1;
        16'h0064: hwregs_rdata <= {31'b0, gpu_busy};
        16'h0080: hwregs_rdata <= gpu_clip_x1;
        16'h0084: hwregs_rdata <= gpu_clip_y1;
        16'h0088: hwregs_rdata <= gpu_clip_x2;
        16'h008C: hwregs_rdata <= gpu_clip_y2;
endcase
end

byte_fifo  uart_tx_fifo (
    .clk(clock),
    .reset(reset),
    .write_enable(hwregs_request && hwregs_write && hwregs_address==16'h0014),
    .write_data(hwregs_wdata[7:0]),
    .read_enable(uart_tx_complete),
    .read_data(uart_tx_word),
    .slots_free(uart_tx_slots_free),
    .pointers(pointers),
    .not_empty(uart_tx_ready)
  );

byte_fifo  uart_rx_fifo (
    .clk(clock),
    .reset(reset),
    .write_enable(uart_rx_complete),
    .write_data(uart_rx_word),
    .read_enable(hwregs_request && !hwregs_write && hwregs_address==16'h0018),
    .read_data(uart_rx_fifo_data),
    .not_empty(uart_rx_fifo_not_empty)
  );

byte_fifo  keyboard_fifo (
    .clk(clock),
    .reset(reset),
    .write_enable(keyboard_strobe),
    .write_data(keyboard_data),
    .read_enable(hwregs_request && !hwregs_write && hwregs_address==16'h0028),
    .read_data(keyboard_fifo_data),
    .not_empty(keyboard_fifo_not_empty)
  );


endmodule