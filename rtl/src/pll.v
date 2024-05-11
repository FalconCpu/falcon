`timescale 1ns/1ns

module pll (
		input  wire  refclk,   //  refclk.clk
		input  wire  rst,      //   reset.reset
		output reg   outclk_0, // outclk0.clk
		output reg   outclk_1, // outclk1.clk
		output reg   locked    //  locked.export
	);

initial begin
    locked <= 0;
    #30;
    locked <= 1;
end

always begin
    outclk_0 <= 0;
    outclk_1 <= 0;
    #5;
    outclk_0 <= 1;
    outclk_1 <= 1;
    #5;
end

endmodule