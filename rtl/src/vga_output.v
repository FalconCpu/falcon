`timescale 1ns/1ns

module vga_output(
    input        clock,
    input        reset,
    input [8:0]  screen_blank,
    input [31:0] screen_address,

    // connections to the pins
    output       VGA_BLANK_N,
	output [7:0] VGA_B,
	output       VGA_CLK,
	output [7:0] VGA_G,
	output       VGA_HS,
	output [7:0] VGA_R,
	output       VGA_SYNC_N,
	output       VGA_VS,

    // connections to the SDRAN
    output        vga_request,
    output [25:0] vga_address,
    input         vga_valid,
    input [31:0]  vga_rdata,
    input         vga_complete,

    // connections to mouse_interface
    input [9:0]   mouse_x,
    input [9:0]   mouse_y
);

reg [1:0] next_div, this_div;
reg [9:0] next_x,  this_x;
reg [9:0] next_y,  this_y;
reg [7:0] next_r,  this_r;
reg [7:0] next_g,  this_g;
reg [7:0] next_b,  this_b;
reg       next_hs, this_hs;
reg       next_vs, this_vs;
reg [25:0]next_addr, this_addr;
reg       next_request, this_request;

reg [31:0] fifo[0:127];
reg [6:0] next_write_ptr, this_write_ptr;
reg [6:0] next_read_ptr, this_read_ptr;
reg [31:0] this_fifo_data;
wire signed [9:0] mouse_dx = this_x - mouse_x;
wire signed [9:0] mouse_dy = this_y - mouse_y;

wire [7:0] this_pixel_index = screen_blank[8] ? screen_blank[7:0] 
                            : (this_x[1:0]==2'h0) ? this_fifo_data[7:0]
                            : (this_x[1:0]==2'h1) ? this_fifo_data[15:8]
                            : (this_x[1:0]==2'h2) ? this_fifo_data[23:16]
                            :                       this_fifo_data[31:24];
reg [23:0] this_pixel_color;

wire [6:0] fifo_space = this_read_ptr - this_write_ptr;

reg [23:0] palette[0:255];
initial 
    $readmemh("palette.hex", palette);



assign VGA_HS  = ! this_hs;
assign VGA_VS  = ! this_vs;
assign VGA_R   =   this_r;
assign VGA_G   =   this_g;
assign VGA_B   =   this_b;
assign VGA_CLK =  this_div[1];
assign VGA_BLANK_N = 1'b1;
assign VGA_SYNC_N = 1'b1;
assign vga_request = this_request;
assign vga_address = this_addr;

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

// Horizontal Timings
// Active Pixels        640   0..639
// Front Porch           16   640..655
// Sync Width            96   656..751
// Back Porch            48   752..799
// Sync Polarity        neg
// 
// Vertical Timings
// Active Lines         480   0..479
// Front Porch           10   480..489
// Sync Width             2   490..491
// Back Porch            33   492..524
// Sync Polarity        neg

always @(*) begin
    next_div = this_div + 1'b1;
    next_x = this_x;
    next_y = this_y;
    next_r = this_r;
    next_g = this_g;
    next_b = this_b;
    next_hs = this_hs;
    next_vs = this_vs;
    next_read_ptr = this_read_ptr;
    next_write_ptr = this_write_ptr;
    next_addr = this_addr;
    next_request = this_request;

    if (this_div==2'd3) begin
        next_x = this_x + 1'b1;
        if (next_x==800) begin
            next_x = 0;
            next_y = this_y + 1'b1;
            if (next_y==525)
                next_y = 0;
        end

        if (this_x>=640 || this_y>=480) begin
            next_r = 0;
            next_g = 0;
            next_b = 0;            
        end else begin
            next_r = this_pixel_color[23:16];
            next_g = this_pixel_color[15:8];
            next_b = this_pixel_color[7:0];
        end

        if (this_x<640 && this_y<480 && mouse_dx>0 && mouse_dx<=15 && mouse_dy>=0 && mouse_dy<=15 && mouse_image_line[15-mouse_dx[3:0]]) begin
            next_r = 255;
            next_g = 255;
            next_b = 255;
        end

        next_hs = (this_x>=656 && this_x<=751);
        next_vs = (this_y>=490 && this_y<=491);
    end

    if (this_div==3 && this_x[1:0]==2'h3 && this_x<640 && this_y<480)
        next_read_ptr = this_read_ptr + 1'b1;

    if ((fifo_space==0 || fifo_space>64) && this_write_ptr[2:0]==0 && (this_y<480 || (this_y==524 && this_x>640)))
        next_request = 1'b1;

    if (vga_complete) begin
        next_request = 1'b0;
        next_addr = this_addr + 8'd32;
    end

    if (vga_valid)
        next_write_ptr = this_write_ptr + 1'b1;

    if (this_vs) begin
        next_read_ptr = 0;
        next_write_ptr = 0;
        next_addr = screen_address;
    end

    if (reset) begin
        next_div = 0;
        next_x = 0;
        next_y = 480;
        next_read_ptr = 0;
        next_write_ptr = 0;
        next_request = 0;
        next_addr = screen_address;
    end
end




always @(posedge clock) begin
    this_div <= next_div;
    this_x   <= next_x;
    this_y   <= next_y;
    this_r   <= next_r;
    this_g   <= next_g;
    this_b   <= next_b;
    this_hs  <= next_hs;
    this_vs  <= next_vs;
    this_read_ptr <= next_read_ptr;
    this_write_ptr <= next_write_ptr;
    if (vga_valid)
        fifo[this_write_ptr] <= vga_rdata;
    this_fifo_data <= fifo[next_read_ptr];
    this_addr <= next_addr;
    this_request <= next_request;
    mouse_image_line <= mouse_image[mouse_dy[3:0]];
    this_pixel_color <= palette[this_pixel_index];
end

endmodule