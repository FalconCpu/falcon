`timescale 1ns/1ns

module vga_output (
    input               clock,          // 100MHz system clock
    input               reset,
    
	output	     	    VGA_CLK,        // 25MHz pixel clock
	output reg [7:0]	VGA_R,          //  Red
	output reg [7:0]	VGA_G,          //  Green
	output reg [7:0]	VGA_B,          //  Blue
	output reg      	VGA_HS,         //  Horizontal sync
	output reg      	VGA_VS,         //  Vertical sync

    // SDRAM interface
    output reg          vga_request,
    output reg [25:0]   vga_address,
    input   [31:0]      vga_rdata,
    input               vga_valid,
    input               vga_complete,
    input               vga_ack,

    // Mouse
    input [9:0]         mouse_x,
    input [9:0]         mouse_y
);

// Clock divider for 25MHz
reg [1:0] clock_div;

// Horizontal timing counters
reg [9:0] h_count;
parameter H_VISIBLE = 640;
parameter H_FRONT_PORCH = 16;
parameter H_SYNC = 96;
parameter H_BACK_PORCH = 48;
parameter H_TOTAL = 800;

// Vertical timing counters  
reg [9:0] v_count;
parameter V_VISIBLE = 10'd480;
parameter V_FRONT_PORCH = 10'd10;
parameter V_SYNC = 10'd2;
parameter V_BACK_PORCH = 10'd33;
parameter V_TOTAL = 10'd525;

parameter SCREEN_START_ADDRESS = 26'h3f80000;
parameter SCREEN_END_ADDRESS = 26'h3f80000 + 640*480*4;

// Fifo for data read from the SDRAM
reg        seen_complete;
reg [31:0] fifo_data [0:255];
reg [7:0]  fifo_rd_ptr;
reg [7:0]  fifo_wr_ptr;
reg [7:0]  fifo_full_slots;

reg [31:0] fifo_rd_data;
reg [7:0]  pixel_color_index;

// Pallette ram
reg [23:0] palette [0:255];
initial begin
    $readmemh("palette.hex", palette);
end
reg [23:0] pixel_color;

// Mouse cursor
wire [9:0] sprite_x = h_count - mouse_x;
wire [9:0] sprite_y = v_count - mouse_y;

reg [15:0] mouse_image[15:0];
initial begin
    mouse_image[0]  = 16'b1000000000000000;
    mouse_image[1]  = 16'b1100000000000000;
    mouse_image[2]  = 16'b1110000000000000;
    mouse_image[3]  = 16'b1111000000000000;
    mouse_image[4]  = 16'b1111100000000000;
    mouse_image[5]  = 16'b1111110000000000;
    mouse_image[6]  = 16'b1111111000000000;
    mouse_image[7]  = 16'b1111111100000000;
    mouse_image[8]  = 16'b1111111110000000;
    mouse_image[9]  = 16'b1111111111000000;
    mouse_image[10] = 16'b1111110000000000;
    mouse_image[11] = 16'b1110011000000000;
    mouse_image[12] = 16'b1100011000000000;
    mouse_image[13] = 16'b1000001100000000;
    mouse_image[14] = 16'b0000001100000000;
    mouse_image[15] = 16'b0000000110000000;
end
reg [15:0] mouse_image_line;
reg        mouse_image_bit;


assign VGA_CLK = ~clock_div[1];

always @(posedge clock) begin
    // Generate 25MHz pixel clock
    clock_div <= clock_div + 1'b1;

    if (clock_div == 2'b11) begin
        // Horizontal & Vertical counters
        if (h_count == H_TOTAL-1) begin
            h_count <= 0;
            if (v_count == V_TOTAL-1)
                v_count <= 0;
            else    
                v_count <= v_count + 1'b1;
        end else
            h_count <= h_count + 1'b1;

        // Generate sync signals  (negative polarity)
        if (h_count == H_VISIBLE + H_FRONT_PORCH)
            VGA_HS <= 0;
        else if (h_count == H_VISIBLE + H_FRONT_PORCH + H_SYNC)
            VGA_HS <= 1;
            
        // Vertical sync (neggative polarity)
        if (v_count == V_VISIBLE + V_FRONT_PORCH)
            VGA_VS <= 0;
        else if (v_count == V_VISIBLE + V_FRONT_PORCH + V_SYNC)
            VGA_VS <= 1;
    end

    // Fetch data from the fifo
    fifo_rd_data <= fifo_data[fifo_rd_ptr];

    // determine color index for the current pixel
    case(h_count[1:0])
        2'b00: pixel_color_index = fifo_rd_data[7:0];
        2'b01: pixel_color_index = fifo_rd_data[15:8];
        2'b10: pixel_color_index = fifo_rd_data[23:16];
        2'b11: pixel_color_index = fifo_rd_data[31:24];
    endcase
    
    pixel_color <= palette[pixel_color_index];

    // Draw mouse cursor
    mouse_image_line <=  mouse_image[sprite_y];
    mouse_image_bit <= mouse_image_line[15-sprite_x];
    if (sprite_x < 16 && sprite_y < 16 && mouse_image_bit)
        pixel_color <= 24'hffffff;

    // convert color index to RGB  (this will be replaced by a color palette later)
    if (h_count < H_VISIBLE && v_count < V_VISIBLE) begin
        VGA_R <= pixel_color[7:0];
        VGA_G <= pixel_color[15:8];
        VGA_B <= pixel_color[23:16];
    end else begin
        VGA_R <= 0;
        VGA_G <= 0;
        VGA_B <= 0;
    end

    // Read data from SDRAM into the fifo
    if (vga_valid) begin
        fifo_data[fifo_wr_ptr] <= vga_rdata;
        fifo_wr_ptr <= fifo_wr_ptr + 1'b1;
        vga_address <= vga_address + 4'd4;
    end


    // Increment the fifo read pointer every 4th visible pixel
    if (clock_div==2'b11 && h_count[1:0]==2'b11 && h_count<H_VISIBLE && v_count<V_VISIBLE)
        fifo_rd_ptr <= fifo_rd_ptr + 1'b1;

    // Request data from SDRAM
    if (vga_ack)
        vga_request <= 0;

    fifo_full_slots = fifo_wr_ptr - fifo_rd_ptr;

    if (reset || (v_count>=V_VISIBLE && v_count<=V_TOTAL-2) || (v_count==V_TOTAL-1 && h_count<512)) begin
        // Blank the fifo and reset the fifo pointers at the end of a frame
        fifo_rd_ptr <= 0;
        fifo_wr_ptr <= 0;
        vga_address <= SCREEN_START_ADDRESS;
        vga_request <= 0;
    end else if (fifo_full_slots<200 && vga_address<SCREEN_END_ADDRESS && h_count[4:0]==5'b0 && clock_div==2'h0) begin
        // Request a new 32 block of data from SDRAM every 32 pixels
        vga_request <= 1;
    end


    if (reset) begin
        clock_div <= 0;
        VGA_HS <= 1;
        VGA_VS <= 1;
        VGA_R <= 0;
        VGA_G <= 0;
        VGA_B <= 0;
        h_count <= 0;
        v_count <= V_TOTAL-2'd3;
        seen_complete <= 1'b0;
    end
end

endmodule

