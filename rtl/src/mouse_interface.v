`timescale 1ns/1ns

module mouse_interface(
    input     clock,     // system clock @ 100Mhz
    input     reset,
    inout     PS2_CLK,
    inout     PS2_DAT,
    output [9:0] mouse_x,
    output [9:0] mouse_y,
    output reg [2:0] mouse_buttons
);

// Gaps between mouse movements are on the order of 730000 clock cycles
// So set a timeout of about 1/2 this to be safe
parameter INTERPACKET_TIMEOUT = 20'd300000;

parameter REQUEST_TO_SEND_TIMER = 20'h2000;

parameter STATE_RESET               = 3'h0;
parameter STATE_TRANSMIT_BIT        = 3'h1;
parameter STATE_RECEIVING           = 3'h3;

reg [20:0] this_timer,       next_timer;
reg [3:0]  this_bit_number,  next_bit_number;
reg        this_drive_clock, next_drive_clock;
reg        this_drive_data,  next_drive_data;
reg [3:0]  this_state,       next_state;
reg        prev_clk,         prev2_clk;
reg [32:0] this_shift_reg,   next_shift_reg;
reg [5:0]  this_shift_count, next_shift_count;

//wire [10:0] message= 10'b01111111111;
wire [10:0] message= 11'b00010111101;

reg signed [13:0] this_x, next_x;
reg signed [13:0] this_y, next_y;
reg        [2:0]  next_mouse_buttons;
reg        [13:0] dx,dy;
wire              clock_falling_edge = prev_clk==0 && prev2_clk==1;

// Received message at the time of the final clock edge
//    2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 
// ST BL BR BM 1  XS YS X8 Y8 PA SP ST X0 X1 X2 X3 X4 X5 X6 X7 PA SP ST Y0 Y1 Y2 Y3 Y4 Y5 Y6 Y7 PA SP

assign PS2_CLK = this_drive_clock ? 1'b0 : 1'bz;
assign PS2_DAT = this_drive_data ? 1'b0 : 1'bz;
assign mouse_x = this_x[12:3];
assign mouse_y = this_y[12:3];

// ------------------------------------------------------------------------
//                        Transmit state machine
// ------------------------------------------------------------------------

always @(*) begin
    next_state = this_state;
    next_timer = this_timer - 1'b1;
    next_drive_clock = 0;
    next_drive_data = 0;
    next_bit_number = this_bit_number;
    next_x = this_x;
    next_y = this_y;
    dx = {{5{this_shift_reg[6]}}, this_shift_reg[8] ^ this_shift_reg[6], this_shift_reg[20:13]};
    dy = {{5{this_shift_reg[7]}}, this_shift_reg[9] ^ this_shift_reg[7], this_shift_reg[31:24]};
    next_mouse_buttons = mouse_buttons;

    if (reset) begin
        next_timer = REQUEST_TO_SEND_TIMER;
        next_bit_number = 10;
        next_state = STATE_RESET;
        next_drive_clock = 0;
        next_drive_data = 0;
        next_x = 0;
        next_y = 0;

    end else if (this_state==STATE_RESET) begin
        next_bit_number = 10;
        next_drive_clock = 1;
        next_drive_data = 1;
        if (this_timer==0)
            next_state = STATE_TRANSMIT_BIT;

    end else if (this_state==STATE_TRANSMIT_BIT) begin
        next_drive_data = ! message[this_bit_number];
        if (clock_falling_edge) begin
            if (this_bit_number==0)
                next_state = STATE_RECEIVING;
            else
                next_bit_number = this_bit_number - 1'b1;
        end

    end else if (this_state==STATE_RECEIVING) begin
        if (PS2_CLK==0)
            next_timer = INTERPACKET_TIMEOUT;
        else
            next_timer = this_timer - 1'b1;
        
    end 

    if (clock_falling_edge) begin
        next_shift_reg = {PS2_DAT,this_shift_reg[32:1]};
        next_shift_count = this_shift_count - 1'b1;

        if (next_shift_count==0) begin
            next_x = this_x + dx;
            next_y = this_y - dy;
            next_mouse_buttons = this_shift_reg[4:2];
        end

    end else begin
        next_shift_reg = this_shift_reg;
        if (this_timer==0)
            next_shift_count = 33;
        else
            next_shift_count = this_shift_count;
    end

    if (next_x < 0)
        next_x = 0;
    if (next_x > 14'd5112)
        next_x = 14'd5112;
    if (next_y < 0)
        next_y = 0;
    if (next_y > 14'd3967)
        next_y = 14'd3967;
end


always @(posedge clock) begin
    this_timer <= next_timer;
    this_bit_number <= next_bit_number;
    this_drive_clock <= next_drive_clock;
    this_drive_data  <= next_drive_data;
    this_state <= next_state;
    prev2_clk <= prev_clk;
    prev_clk  <= PS2_CLK;
    this_x    <= next_x;
    this_y    <= next_y;
    mouse_buttons <= next_mouse_buttons;
    this_shift_reg <= next_shift_reg;
    this_shift_count <= next_shift_count;
end

endmodule