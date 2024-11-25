`timescale 1ns/1ns

module  pll(
	input  refclk,
	input  rst,
	output reg outclk_0,
	output reg outclk_1,
	output reg locked
);

initial begin
    locked <= 1'b0;
    #26;
    locked <= 1'b1;
end

always begin
    outclk_0 <= 1'b0;
    outclk_1 <= 1'b0;
    #5;
    outclk_0 <= 1'b1;
    outclk_1 <= 1'b1;
    #5;
end


endmodule