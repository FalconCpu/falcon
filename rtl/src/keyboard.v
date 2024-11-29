`timescale 1ns/1ns

module keyboard_if(
    input             clock,    // 100 Mhz clock
    input             reset,
    
	input    	      PS2_CLK2,
	input    	      PS2_DAT2,

    output reg[7:0]  keyboard_code,
    output reg       keyboard_strobe
);

reg [1:0]    this_state, next_state;
reg [7:0]    this_shift_reg, next_shift_reg;
reg [3:0]    this_count, next_count;
reg [13:0]   this_timeout, next_timeout;
reg          this_ps2_clk, this_ps2_dat, prev_ps2_clk;
reg [7:0]    next_keyboard_code;
reg          next_keyboard_strobe;

always @(*) begin
    next_shift_reg = this_shift_reg;
    next_keyboard_code = keyboard_code;
    next_keyboard_strobe = 1'b0;
    next_timeout = this_timeout - 1'b1;
    next_count   = this_count;

    if (reset || this_timeout==0) begin
        next_shift_reg = 0;
        next_count = 0;
        next_timeout = 13'h1fff;
    end else if (this_ps2_clk==1'b0 && prev_ps2_clk==1'b1) begin
        // got falling edge of clock
        next_shift_reg = {this_ps2_dat, this_shift_reg[7:1]};
        next_count = this_count + 1'b1;
        next_timeout = 13'h1fff;
        if (this_count==9) begin
            next_keyboard_code = this_shift_reg;
            next_keyboard_strobe = 1'b1;
        end else if (this_count==10) begin
            next_count = 0;
        end
    end
end

always @(posedge clock) begin
    this_shift_reg  <= next_shift_reg;
    this_count      <= next_count;
    this_timeout    <= next_timeout;
    this_ps2_clk    <= PS2_CLK2;
    this_ps2_dat    <= PS2_DAT2;
    prev_ps2_clk    <= this_ps2_clk;
    keyboard_code   <= next_keyboard_code;
    keyboard_strobe <= next_keyboard_strobe;
end


endmodule